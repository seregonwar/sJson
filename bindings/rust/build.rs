use std::{env, path::PathBuf, process::Command};

fn run(command: &mut Command) {
    let status = command
        .status()
        .expect("could not start CMake; install CMake and a C compiler");
    assert!(status.success(), "native sJson build failed");
}

fn visual_studio_generator(cmake: &std::ffi::OsStr) -> String {
    let installer =
        PathBuf::from(env::var_os("ProgramFiles(x86)").expect("missing ProgramFiles(x86)"))
            .join("Microsoft Visual Studio/Installer/vswhere.exe");
    let version = Command::new(installer)
        .args([
            "-latest",
            "-products",
            "*",
            "-requires",
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "-property",
            "installationVersion",
            "-utf8",
        ])
        .output()
        .expect("Visual Studio detection failed; set CMAKE_GENERATOR explicitly");
    assert!(version.status.success(), "vswhere failed");
    let version = String::from_utf8(version.stdout).expect("invalid Visual Studio version");
    let major = version
        .trim()
        .split('.')
        .next()
        .filter(|s| !s.is_empty())
        .expect("no Visual Studio C++ toolchain found; set CMAKE_GENERATOR explicitly");
    let prefix = format!("Visual Studio {major} ");
    let help = Command::new(cmake)
        .arg("--help")
        .output()
        .expect("cannot query CMake generators");
    String::from_utf8_lossy(&help.stdout).lines()
        .map(|line| line.trim_start_matches(['*', ' ']))
        .filter_map(|line| line.split_once('=').map(|(name, _)| name.trim()))
        .find(|name| name.starts_with(&prefix))
        .map(str::to_owned)
        .expect("CMake does not support the installed Visual Studio; update CMake or set CMAKE_GENERATOR")
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
        configure
            .arg("-G")
            .arg(visual_studio_generator(&cmake))
            .args(["-A", architecture]);
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
