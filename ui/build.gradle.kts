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
    implementation("org.json:json:20231013")
    implementation("io.github.andrewauclair:modern-docking-api:0.10.0-SNAPSHOT")
    implementation("io.github.andrewauclair:modern-docking-single-app:0.10.0-SNAPSHOT")
    implementation("io.github.andrewauclair:modern-docking-ui:0.10.0-SNAPSHOT")
    implementation("com.formdev:flatlaf:3.2")
    implementation("com.formdev:flatlaf-extras:3.2")
}

tasks.test {
    useJUnitPlatform()
}
