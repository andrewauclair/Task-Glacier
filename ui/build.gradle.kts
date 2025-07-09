plugins {
    java
    application
    id("org.gradlex.extra-java-module-info") version "1.12"
    id("org.gradlex.java-module-dependencies") version "1.9.2"
    id("org.beryx.jlink") version "3.1.1"
}

repositories {
    mavenLocal()
    mavenCentral()
    maven { url = uri("https://s01.oss.sonatype.org/content/repositories/snapshots/") }
}

sourceSets.main {
    resources {
        srcDirs("resources")
    }
}

dependencies {
    testImplementation(platform("org.junit:junit-bom:5.9.1"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    implementation("io.github.andrewauclair:modern-docking-api:1.2.0")
    implementation("io.github.andrewauclair:modern-docking-single-app:1.2.0")
    implementation("io.github.andrewauclair:modern-docking-ui:1.2.0")
    implementation("com.formdev:flatlaf:3.5.4")
    implementation("com.formdev:flatlaf-extras:3.5.4")
    implementation("me.xdrop:fuzzywuzzy:1.4.0")

    // https://mvnrepository.com/artifact/org.swinglabs/swingx
    implementation("org.swinglabs:swingx:1.6.1")
}

extraJavaModuleInfo {
    automaticModule("swingx-1.6.1.jar", "org.swinglabs.swingx")
    automaticModule("swing-worker-1.1.jar", "org.swinglabs.swingx")
    automaticModule("filters-2.0.235.jar", "com.jhlabs.filters")
}

tasks.test {
    useJUnitPlatform()
}

application {
    mainClass = "taskglacier.MainFrame"
    mainModule = "task.glacier.ui"
}

//tasks.withType<Jar> {
//    manifest {
//        attributes["Main-Class"] = "taskglacier.MainFrame"
//    }
//    from(configurations.runtimeClasspath.get().map({ if (it.isDirectory) it else zipTree(it) }))
//    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
//}

jlink {
    mergedModule {
//        requires("com.formdev.flatlaf")
//        requires("modern_docking.api")
//        requires("modern_docking.single_app")
//        requires("modern_docking.ui_ext")
    }
    options.set(listOf("--strip-debug", "--compress", "2", "--no-header-files", "--no-man-pages"))


    jpackage {
        if (org.gradle.internal.os.OperatingSystem.current().isWindows) {
            installerOptions = listOf("--win-per-user-install", "--win-dir-chooser", "--win-menu", "--verbose")
        }
    }
}