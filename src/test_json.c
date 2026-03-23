#define JSON_IMPLEMENTATION
#include "json_pal.h"
#include <stdio.h>
#include <string.h>

#define T(name) printf("[TEST] %-52s", name)
#define PASS()  printf("PASS\n")
#define FAIL(m) do { printf("FAIL: %s\n", m); return 1; } while(0)

static int test_primitives(void) {
    JsonArena* a = json_arena_create(NULL, 4096);
    JsonError err; JsonValue* v;
    bool b; int64_t i; double f; const char* s; uint32_t sl;

    T("null");          v=json_parse_cstr(a,"null",&err);  if(!v||v->type!=JSON_NULL) FAIL("null"); PASS();
    T("true/false");    v=json_parse_cstr(a,"true",&err);  json_get_bool(v,&b); if(!b) FAIL("true");
                        v=json_parse_cstr(a,"false",&err); json_get_bool(v,&b); if(b)  FAIL("false"); PASS();
    T("integer");       v=json_parse_cstr(a,"-9223372036854775807",&err);
                        json_get_int(v,&i); if(i!=-9223372036854775807LL) FAIL("int"); PASS();
    T("float");         v=json_parse_cstr(a,"3.14159265358979",&err);
                        json_get_float(v,&f); if(f<3.14||f>3.15) FAIL("float"); PASS();
    T("string escapes");v=json_parse_cstr(a,"\"hello\\nworld\\t!\"",&err);
                        json_get_string(v,&s,&sl);
                        if(sl!=13||memcmp(s,"hello\nworld\t!",13)) FAIL("escape content"); PASS();

    /* Bug #2 regression: \u00e9 must be 2 bytes (C3 A9), not 3 */
    T("\\u00e9 exact 2 bytes");
                        v=json_parse_cstr(a,"\"\\u00e9\"",&err);
                        json_get_string(v,&s,&sl);
                        if(sl!=2) FAIL("u00e9 len wrong"); PASS();

    /* \u00e9 (2 bytes) + \u4e2d (3 bytes) = 5 bytes total */
    T("\\u00e9\\u4e2d exact 5 bytes");
                        v=json_parse_cstr(a,"\"\\u00e9\\u4e2d\"",&err);
                        json_get_string(v,&s,&sl);
                        if(sl!=5) FAIL("combined uXXXX len wrong"); PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_objects(void) {
    JsonArena* a = json_arena_create(NULL, 8192);
    JsonError err; JsonValue *v, *item; int64_t iv; uint32_t len;

    /* Bug #1 regression */
    T("simple object {\"a\":1,\"b\":2}");
                        v=json_parse_cstr(a,"{\"a\":1,\"b\":2}",&err);
                        if(!v||err!=JSON_OK) FAIL("parse failed");
                        json_obj_finalize(a,v);
                        json_obj_get(v,"a",&item); json_get_int(item,&iv); if(iv!=1) FAIL("a!=1");
                        json_obj_get(v,"b",&item); json_get_int(item,&iv); if(iv!=2) FAIL("b!=2");
                        PASS();

    T("empty object {}");
                        v=json_parse_cstr(a,"{}",&err);
                        if(!v||v->type!=JSON_OBJECT) FAIL("{}"); PASS();

    T("nested {\"users\":[{\"name\":\"Alice\"}]}");
                        v=json_parse_cstr(a,"{\"users\":[{\"name\":\"Alice\",\"age\":30}]}",&err);
                        if(!v||err!=JSON_OK) FAIL("nested parse");
                        json_obj_finalize(a,v);
                        JsonValue *users, *user0, *name_v;
                        json_obj_get(v,"users",&users);
                        json_arr_get(users,0,&user0);
                        json_obj_finalize(a,user0);
                        json_obj_get(user0,"name",&name_v);
                        const char* nm; uint32_t nl;
                        json_get_string(name_v,&nm,&nl);
                        if(nl!=5||memcmp(nm,"Alice",5)) FAIL("nested name"); PASS();

    T("array [1,2,3]");  v=json_parse_cstr(a,"[1,2,3]",&err);
                        json_get_arr_len(v,&len); if(len!=3) FAIL("arr len");
                        json_arr_get(v,2,&item); json_get_int(item,&iv); if(iv!=3) FAIL("arr[2]"); PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_path(void) {
    JsonArena* a = json_arena_create(NULL, 8192);
    JsonError err; JsonValue *root, *v; int64_t iv;

    root=json_parse_cstr(a,"{\"config\":{\"host\":\"localhost\",\"port\":8080},\"data\":[10,20,30]}",&err);
    if(!root){printf("[TEST] path query setup                                       FAIL: parse\n"); return 1;}
    json_obj_finalize(a,root);
    JsonValue* cfg; json_obj_get(root,"config",&cfg); json_obj_finalize(a,cfg);

    T("path data[2]");   v=json_path(root,"data[2]",&err);
                        json_get_int(v,&iv); if(iv!=30) FAIL("data[2]"); PASS();
    T("path config.port");v=json_path(root,"config.port",&err);
                        json_get_int(v,&iv); if(iv!=8080) FAIL("port"); PASS();
    T("path config[\"host\"]");
                        v=json_path(root,"config[\"host\"]",&err);
                        const char* s; uint32_t sl;
                        json_get_string(v,&s,&sl);
                        if(sl!=9||memcmp(s,"localhost",9)) FAIL("host"); PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_serialize(void) {
    JsonArena* a = json_arena_create(NULL, 8192);
    JsonError err; JsonValue* root; char buf[1024];

    T("round-trip {x,y,z}");
                        root=json_parse_cstr(a,"{\"x\":42,\"y\":[1,2,3],\"z\":true}",&err);
                        if(!root||err!=JSON_OK) FAIL("rt parse");
                        json_obj_finalize(a,root);
                        size_t written;
                        err=json_write(root,buf,sizeof(buf),&written,NULL);
                        if(err!=JSON_OK) FAIL("serialize");
                        JsonArena* a2=json_arena_create(NULL,4096);
                        JsonValue* r2=json_parse_cstr(a2,buf,&err);
                        if(!r2||err!=JSON_OK) FAIL("re-parse");
                        json_arena_destroy(a2); PASS();

    T("buffer too small returns correct error");
                        root=json_parse_cstr(a,"{\"key\":\"value\"}",&err);
                        if(!root) FAIL("parse for buf-small test");
                        char tiny[5];
                        JsonError e2=json_write(root,tiny,sizeof(tiny),NULL,NULL);
                        if(e2!=JSON_ERR_BUFFER_TOO_SMALL) FAIL("wrong error code"); PASS();

    T("pretty print");  {
                        JsonWriteOpts opts={0}; opts.pretty=true; opts.indent=2;
                        err=json_write(root,buf,sizeof(buf),&written,&opts);
                        if(err!=JSON_OK||written<5) FAIL("pretty"); } PASS();

    T("measure == write len");
                        { size_t sz=0; json_measure(root,&sz,NULL);
                        err=json_write(root,buf,sz+2,&written,NULL);
                        if(err!=JSON_OK||written!=sz) FAIL("measure"); } PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_construction(void) {
    JsonArena* a = json_arena_create(NULL, 16384);
    JsonValue* arr=json_make_array(a);
    JsonValue* obj=json_make_object(a);
    int i; uint32_t len;

    T("build 100-element array");
                        for(i=0;i<100;i++) json_arr_push(arr,a,json_make_int(a,i));
                        json_get_arr_len(arr,&len); if(len!=100) FAIL("arr len"); PASS();

    T("build object + finalize");
                        json_obj_setz(obj,a,"pi",json_make_float(a,3.14159));
                        json_obj_setz(obj,a,"name",json_make_stringz(a,"test"));
                        json_obj_setz(obj,a,"ok",json_make_bool(a,true));
                        json_obj_finalize(a,obj);
                        JsonValue* v; if(json_obj_get(obj,"pi",&v)!=JSON_OK) FAIL("pi"); PASS();

    T("deep clone == original");
                        JsonArena* a2=json_arena_create(NULL,8192);
                        JsonValue* cl=json_clone(a2,obj);
                        if(!cl||!json_equal(obj,cl)) FAIL("clone");
                        json_arena_destroy(a2); PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_errors(void) {
    JsonArena* a=json_arena_create(NULL,4096);
    JsonError err; JsonValue* v;

    T("reject {bad}");  v=json_parse_cstr(a,"{bad}",&err);   if(v)  FAIL("accepted bad"); PASS();
    T("reject [01]");   v=json_parse_cstr(a,"[01]",&err);    if(v)  FAIL("leading zero"); PASS();
    T("reject \\q");    v=json_parse_cstr(a,"\"\\q\"",&err); if(v)  FAIL("bad escape");   PASS();
    T("reject trailing");v=json_parse_cstr(a,"42 garbage",&err); if(v) FAIL("trailing");  PASS();
    T("reject \\uD800");v=json_parse_cstr(a,"\"\\uD800\"",&err); if(v) FAIL("lone surr"); PASS();
    T("type mismatch"); v=json_parse_cstr(a,"42",&err);
                        bool b; if(json_get_bool(v,&b)!=JSON_ERR_TYPE_MISMATCH) FAIL("mismatch"); PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_introsort(void) {
    JsonArena* a=json_arena_create(NULL,512*1024);
    JsonValue* obj=json_make_object(a);
    char kb[32]; int n=200, i;

    T("introsort 200 keys + binary search");
                        for(i=0;i<n;i++){
                            snprintf(kb,sizeof(kb),"key_%04d",i);
                            json_obj_setz(obj,a,kb,json_make_int(a,(int64_t)i));
                        }
                        json_obj_finalize(a,obj);
                        JsonValue* v; int64_t iv;
                        snprintf(kb,sizeof(kb),"key_%04d",99);
                        if(json_obj_get(obj,kb,&v)!=JSON_OK) FAIL("lookup 99");
                        json_get_int(v,&iv); if(iv!=99) FAIL("value 99");
                        snprintf(kb,sizeof(kb),"key_%04d",0);
                        if(json_obj_get(obj,kb,&v)!=JSON_OK) FAIL("lookup 0");
                        snprintf(kb,sizeof(kb),"key_%04d",n-1);
                        if(json_obj_get(obj,kb,&v)!=JSON_OK) FAIL("lookup last"); PASS();

    json_arena_destroy(a);
    return 0;
}

static int test_arena_reset(void) {
    JsonArena* a=json_arena_create(NULL,4096);
    JsonError err; int i;

    T("arena reset x5 reuse");
                        for(i=0;i<5;i++){
                            json_arena_reset(a);
                            JsonValue* v=json_parse_cstr(a,"[1,2,3,{\"ok\":true}]",&err);
                            if(!v||err!=JSON_OK) FAIL("reset round");
                        } PASS();

    json_arena_destroy(a);
    return 0;
}

int main(void) {
    int f=0;
    printf("=== json_pal.h v1.1 — regression suite ===\n\n");
    f+=test_primitives();
    f+=test_objects();
    f+=test_path();
    f+=test_serialize();
    f+=test_construction();
    f+=test_errors();
    f+=test_introsort();
    f+=test_arena_reset();
    printf("\n%s — %d failure(s)\n", f?"TESTS FAILED":"ALL TESTS PASSED", f);
    return f?1:0;
}
