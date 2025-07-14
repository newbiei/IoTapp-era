plugins {
    id("com.android.application")
    id("com.google.gms.google-services")
}

android {
    namespace = "com.wera.app"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.wera.app"
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }
    buildFeatures {
        viewBinding = true
    }
    buildFeatures {
        buildConfig = true
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {
        implementation(platform("com.google.firebase:firebase-bom:33.16.0"))
        implementation ("com.google.firebase:firebase-messaging")
        implementation("com.google.firebase:firebase-analytics")
        implementation ("com.journeyapps:zxing-android-embedded:4.3.0")


        implementation("androidx.cardview:cardview:1.0.0")
        implementation("androidx.recyclerview:recyclerview:1.4.0")
        implementation("com.google.android.material:material:1.12.0")
        implementation("androidx.constraintlayout:constraintlayout:2.2.1")
        implementation("androidx.core:core-ktx:1.10.1")
        implementation("androidx.appcompat:appcompat:1.6.1")
        implementation("androidx.fragment:fragment-ktx:1.8.8")
        implementation("androidx.appcompat:appcompat:1.7.1")

        // Room - pilih satu versi
        implementation("androidx.room:room-runtime:2.6.0")
        implementation("androidx.room:room-ktx:2.6.0")
        annotationProcessor("androidx.room:room-compiler:2.6.0") // atau annotationProcessor

        // Navigation (kalau lo pakai)
        implementation("androidx.navigation:navigation-fragment:2.6.0")
        implementation("androidx.navigation:navigation-ui:2.6.0")

        testImplementation("junit:junit:4.13.2")
        androidTestImplementation("androidx.test.ext:junit:1.1.5")
        androidTestImplementation("androidx.test.espresso:espresso-core:3.5.1")
    }