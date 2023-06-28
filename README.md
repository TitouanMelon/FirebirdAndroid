# FirebirdAndroid

# INSTALL
First create a new project, pour le reste de cette explication le nom de projet en minuscules sera appele YourProjectName

When your project is load you need to add C++ in your application :

PHOTO AND EXPLICATION

## Move file in your project

Move all files and folders of the cpp folders inside your cpp folder wich can be found under **YourAndroidApplicationFolder/app/src/main/**

Move Firebird.kt file inside the java folder of your application wich can be found under **YourAndroidApplicationFolder/app/src/main/java/com/example/YourProjectName/**

Move .aar file inside the libs folder of your application wich can be found under **YourAndroidApplicationFolder/app/libs/**

After that your project must contains the files like this :

PHOTO OF FOLDER'S TREE

## Edit files

### CMAKEList.txt

In **add_library()** add at the end before the **)** :

```
firebirdCore.cpp
request.cpp
```

Under **find_library()** add this and replace ProjectName by your YourProjectName :

```
target_include_directories(
        <Project name> PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
```

### firebirdCore.cpp

In this file change the name of JNIEXPORT function by replacing PROJECTNAME by your YourProjectName (there are 5 functions)

```
//For project name firedroid
extern "C" JNIEXPORT jint JNICALL Java_com_example_PROJECTNAME_Firebird_getApiVersion(JNIEnv* env, jobject self)
//become
extern "C" JNIEXPORT jint JNICALL Java_com_example_firedroid_Firebird_getApiVersion(JNIEnv* env, jobject self)
```

### Kotlin files
First of all at the end of the first line of each file add your YourProjectName after the .

Then in Firebird.kt in init function update your YourProjectName in System.loadLibrary("ProjectName")

```
//For project name firedroid
package com.example.firedroid
and
System.loadLibrary("ProjectName")
//become
System.loadLibrary("firedroid")
```

### AndroidManifest.xml

Before application tag insert this :

```
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.MANAGE_EXTERNAL_STORAGE" />
```

### build.gradle of app

found the line with cppFlags and add after '-std=c++17'
the line looks like this after change
```
cppFlags '-std=c++17'
```
in dependencies add the aar file by add this line inside brackets :

```
implementation files('libs/Firebird-3.0.0-android-embedded.aar')
```

# ADD DEBUG INTERFACE

## Add jetpack compose

Then in application tag add activity :

```
<activity
	android:name=".MainActivity"
        android:exported="true"
        android:theme="@style/Theme.AppCompat">
        <intent-filter>
        	<action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
	</intent-filter>
</activity>
```

Inside the android brackets add this

```
buildFeatures {
        compose true
    }
    composeOptions {
        kotlinCompilerExtensionVersion '1.4.6'
    }
```
Inside the dependencies brackets add this

```
implementation 'androidx.activity:activity-compose:1.7.2'
    implementation "androidx.compose.animation:animation:1.4.3"
    implementation "androidx.compose.foundation:foundation:1.4.3"
    implementation 'androidx.compose.material3:material3'
    implementation "androidx.compose.runtime:runtime:1.4.3"
    implementation "androidx.compose.ui:ui:1.4.3"
```

# Use the API with your application
## ADD to your java file
## Add to your cpp file
