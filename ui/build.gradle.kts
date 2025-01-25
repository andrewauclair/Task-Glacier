plugins {
    id("java")
}

repositories {
    mavenCentral()
    maven { url = uri("https://s01.oss.sonatype.org/content/repositories/snapshots/") }
}

dependencies {
    testImplementation(platform("org.junit:junit-bom:5.9.1"))
    testImplementation("org.junit.jupiter:junit-jupiter")
    implementation("io.github.andrewauclair:modern-docking-api:1.0")
    implementation("io.github.andrewauclair:modern-docking-single-app:1.0")
    implementation("io.github.andrewauclair:modern-docking-ui:1.0")
    implementation("com.formdev:flatlaf:3.2")
    implementation("com.formdev:flatlaf-extras:3.2")

    // https://mvnrepository.com/artifact/org.swinglabs/swingx
    implementation("org.swinglabs:swingx:1.6.1")
}

tasks.test {
    useJUnitPlatform()
}
