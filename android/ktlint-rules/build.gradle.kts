plugins {
    alias(libs.plugins.kotlin.jvm)
}

kotlin {
    jvmToolchain(17)
}

dependencies {
    compileOnly(libs.ktlint.cli.ruleset.core)
    compileOnly(libs.ktlint.rule.engine.core)

    testImplementation(libs.junit)
    testImplementation(libs.ktlint.cli.ruleset.core)
    testImplementation(libs.ktlint.rule.engine.core)
    testImplementation(libs.ktlint.test)
    testRuntimeOnly("ch.qos.logback:logback-classic:1.3.14")
}
