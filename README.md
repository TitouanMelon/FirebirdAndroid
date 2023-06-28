# FirebirdAndroid

First create a new project, pour le reste de cette explication le nom de projet en minuscules sera appele ProjectName

When your project is load you need to add C++ in your application :

PHOTO AND EXPLICATION

# Move file in your project

move all files and folders of the cpp folders inside your cpp folder wich can be found under <YourAndroidApplicationFolder>/app/src/main/
move kotlin file inside the java folder inside your java folder wich can be found under <YourAndroidApplicationFolder>/app/src/main/java/com/example/<ProjectName>/

# Edit files

## CMAKEList.txt

In add_library session add at the end before the ) :

```
firebirdCore.cpp
request.cpp file
```

Under find_library session add this and replace <ProjectName> by your ProjectName :

```
target_include_directories(
        <Project name> PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
```

## firebirdCore.cpp

In this file change the name of JNIEXPORT function by replacing PROJETNAME by your ProjectName

## Kotlin files
First of all at the end of the first line of each file add your ProjectName after the .

Then in Firebird.kt in init function add your lib name in System.loadLibrary(name>)

in manifest add before application

```
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.MANAGE_EXTERNAL_STORAGE" />
```

in application add activity

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

add .arr in app/libs and add <implementation files('libs/Name_of_aar')> then sync
add -std=c++17 in cppFlags




for jetpack add this in graddle

```
buildFeatures {
        compose true
    }
    composeOptions {
        kotlinCompilerExtensionVersion '1.4.6'
    }

implementation 'androidx.activity:activity-compose:1.7.2'
    implementation "androidx.compose.animation:animation:1.4.3"
    implementation "androidx.compose.foundation:foundation:1.4.3"
    implementation 'androidx.compose.material3:material3'
    implementation "androidx.compose.runtime:runtime:1.4.3"
    implementation "androidx.compose.ui:ui:1.4.3"
```
