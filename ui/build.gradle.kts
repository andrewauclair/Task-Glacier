plugins {
    java
    application
}

repositories {
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
    implementation("io.github.andrewauclair:modern-docking-api:1.1.4")
    implementation("io.github.andrewauclair:modern-docking-single-app:1.1.4")
    implementation("io.github.andrewauclair:modern-docking-ui:1.1.4")
    implementation("com.formdev:flatlaf:3.5.4")
    implementation("com.formdev:flatlaf-extras:3.5.4")
    implementation("me.xdrop:fuzzywuzzy:1.4.0")

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
