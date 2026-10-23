use std::{env, path::PathBuf, process::Command};

fn run(command: &mut Command) {
    let status = command
        .status()
        .expect("could not start CMake; install CMake and a C compiler");
    assert!(status.success(), "native sJson build failed");
}

fn main() {
    let root = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap()).join("../..");
    let target_env = env::var("CARGO_CFG_TARGET_ENV").unwrap_or_default();
    let output =
        PathBuf::from(env::var_os("OUT_DIR").unwrap()).join(format!("native-{target_env}"));
    let cmake = env::var_os("CMAKE").unwrap_or_else(|| "cmake".into());
    let mut configure = Command::new(&cmake);
    if target_env == "msvc" && env::var_os("CMAKE_GENERATOR").is_none() {
        let architecture = match env::var("CARGO_CFG_TARGET_ARCH").unwrap().as_str() {
            "x86" => "Win32",
            "aarch64" => "ARM64",
            _ => "x64",
        };
        configure.args(["-G", "Visual Studio 17 2022", "-A", architecture]);
    }
    configure.arg("-S").arg(&root).arg("-B").arg(&output).args([
        "-DCMAKE_BUILD_TYPE=Release",
        "-DSJSON_BUILD_TESTS=OFF",
        "-DSJSON_BUILD_BENCHMARKS=OFF",
        "-DSJSON_BUILD_FFI=ON",
        "-DBUILD_SHARED_LIBS=OFF",
    ]);
    if let Some(toolchain) = env::var_os("SJSON_CMAKE_TOOLCHAIN_FILE") {
        configure.arg(format!(
            "-DCMAKE_TOOLCHAIN_FILE={}",
            PathBuf::from(toolchain).display()
        ));
    } else {
        assert_eq!(
            env::var("HOST").unwrap(),
            env::var("TARGET").unwrap(),
            "cross-compiling requires SJSON_CMAKE_TOOLCHAIN_FILE"
        );
    }
    run(&mut configure);
    run(Command::new(&cmake).arg("--build").arg(&output).args([
        "--config",
        "Release",
        "--target",
        "sjson_ffi",
    ]));
    println!("cargo:rustc-link-search=native={}", output.display());
    println!(
        "cargo:rustc-link-search=native={}",
        output.join("Release").display()
    );
    println!("cargo:rustc-link-lib=static=sjson_ffi");
    if env::var("CARGO_CFG_TARGET_FAMILY").unwrap() == "unix" {
        println!("cargo:rustc-link-lib=m");
    }
    for file in [
        "src/sJson.c",
        "src/json_pal.h",
        "bindings/ffi/sjson_ffi.c",
        "bindings/ffi/sjson_ffi.h",
        "CMakeLists.txt",
    ] {
        println!("cargo:rerun-if-changed={}", root.join(file).display());
    }
    println!("cargo:rerun-if-env-changed=SJSON_CMAKE_TOOLCHAIN_FILE");
    println!("cargo:rerun-if-env-changed=CMAKE");
    println!("cargo:rerun-if-env-changed=CMAKE_GENERATOR");
}
