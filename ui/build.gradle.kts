plugins {
    java
    application
}

val gitVersion: String = try {
    ProcessBuilder("git", "describe", "--tags", "--always")
        .directory(rootDir)
        .start()
        .inputStream.bufferedReader().readLine()?.trim() ?: "unknown"
} catch (e: Exception) {
    "unknown"
}

val appVersion = System.getenv("RELEASE_VERSION") ?: gitVersion

repositories {
    mavenCentral()
    maven { url = uri("https://s01.oss.sonatype.org/content/repositories/snapshots/") }
}

val generateVersionFile by tasks.registering {
    val outputDir = layout.buildDirectory.dir("generated-resources")
    outputs.dir(outputDir)
    doLast {
        val file = outputDir.get().file("version.properties").asFile
        file.parentFile.mkdirs()
        file.writeText("version=$appVersion\n")
    }
}

sourceSets.main {
    resources {
        srcDirs("resources", layout.buildDirectory.dir("generated-resources"))
    }
}

tasks.processResources {
    dependsOn(generateVersionFile)
}

dependencies {
    testImplementation(platform("org.junit:junit-bom:5.9.1"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
    implementation("io.github.andrewauclair:modern-docking-api:1.4")
    implementation("io.github.andrewauclair:modern-docking-single-app:1.4")
    implementation("io.github.andrewauclair:modern-docking-ui:1.4")
    implementation("com.formdev:flatlaf:3.6.1")
    implementation("com.formdev:flatlaf-extras:3.6.1")
    implementation("com.formdev:flatlaf-intellij-themes:3.6.1")
    implementation("me.xdrop:fuzzywuzzy:1.4.0")
    implementation("io.github.dj-raven:swing-datetime-picker:2.1.3")

    // https://mvnrepository.com/artifact/org.swinglabs/swingx
    implementation("org.swinglabs:swingx:1.6.1")
}

tasks.test {
    useJUnitPlatform()
}

tasks.withType<Jar> {
    manifest {
        attributes["Main-Class"] = "taskglacier.MainFrame"
    }
    from(configurations.runtimeClasspath.get().map({ if (it.isDirectory) it else zipTree(it) }))
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
}
