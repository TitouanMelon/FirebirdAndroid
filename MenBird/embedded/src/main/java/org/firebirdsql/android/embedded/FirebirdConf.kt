package org.firebirdsql.android.embedded

import android.content.Context
import android.system.ErrnoException
import android.system.Os
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import android.os.Build
import android.util.Log

/**
 * Extract the asset from AAR to device to have firebird work
 */
public object FirebirdConf {
    private val arch = Build.SUPPORTED_ABIS[0] /** @property arch - get the ABI of device to get the correct version of libs */

    /**
     * Extract assets in app's folder
     * @param context - use to get app's folder
     * @param force - if true overwrite file if already exist
     */
    @JvmStatic
    @Throws(IOException::class)
    public fun extractAssets(context: Context, useUsersFolders : Boolean = true, force: Boolean = true) {
        Log.d("AAR", "Moving databases")
        if (useUsersFolders)
            _mvAssetFolder(context, context.getExternalFilesDir(null)!!.absoluteFile, "databases", "") //Move asset/database to app's external folder
        else
            _mvAssetFolder(context, context.filesDir, "databases", "") //Move asset/database to app's folder

        Log.d("AAR", "Moving firebird")
        val firebirdRootPath = File(context.filesDir, "firebird") //Look if firebird folder exist on the device
        if (!force && firebirdRootPath.exists()) //If firebird folder exist and we don't want overwrite return
            return
        else if (firebirdRootPath.exists()) //else delete firebird folder
            _deleteDirectory(firebirdRootPath)

        val firebirdTempRootPath = File(context.filesDir, "firebird.tmp")
        val firebirdTmpPath = File(firebirdTempRootPath, "tmp")
        val firebirdLockPath = File(firebirdTempRootPath, "lock")

        if (firebirdTempRootPath.exists()) //If temporary firebird folder exist delete if
            _deleteDirectory(firebirdTempRootPath)

        firebirdTempRootPath.mkdir() //Create temporary firebird folder
        firebirdTmpPath.mkdirs() //Create tmp folder
        firebirdLockPath.mkdirs() //Create lock folder

        _mvAssetFolder(context, firebirdTempRootPath, "firebird_$arch", "") //Move asset/firebird in temporary firebird

        firebirdTempRootPath.renameTo(firebirdRootPath) //Move temporary firebird to firebird
    }

    /**
     * Set env variable
     * @param context - use to get app's folder
     */
    @JvmStatic
    @Throws(ErrnoException::class)
    public fun setEnv(context: Context) {
        val firebirdRootPath = File(context.filesDir, "firebird")
        val firebirdLibPath = File(firebirdRootPath, "lib")
        val firebirdTmpPath = File(firebirdRootPath, "tmp")
        val firebirdLockPath = File(firebirdRootPath, "lock")

        Os.setenv("FIREBIRD", firebirdRootPath.absolutePath, true)
        Os.setenv("FIREBIRD_LIB", firebirdLibPath.absolutePath, true)
        Os.setenv("FIREBIRD_TMP", firebirdTmpPath.absolutePath, true)
        Os.setenv("FIREBIRD_LOCK", firebirdLockPath.absolutePath, true)

    }

    /**
     * Delete directory
     * @param directory - Directory to delete
     */
    private fun _deleteDirectory(directory: File) {
        for (file in directory.listFiles()!!) {
            if (file.isDirectory)
                _deleteDirectory(file)
            else
                file.delete()
        }
    }

    /**
     * Copy files and subfolders from assetPath to rootDevicePath/devicePath
     * @param context - use to get the assets folder
     * @param RootDevicePath - Root folder where you want your file move in
     * @param assetPath - root path of folder you want move
     * @param devicePath - path in device folder use for recurssion call must be null string in first call
     */
    private fun _mvAssetFolder(context: Context, RootDevicePath: File, assetPath : String, devicePath : String) {
        val assetManager = context.assets
        val buffer = ByteArray(1024)

        Log.d("AAR", "we move from $assetPath/* to ${RootDevicePath.absolutePath}/$devicePath/")

        for (asset in assetManager.list(assetPath)!!) //Get list of asset in assets/assetPath/
        {
            Log.d("AAR","we move $assetPath/$asset to ${RootDevicePath.absolutePath}/$devicePath/$asset")

            val assetDevicePath = File(RootDevicePath, "$devicePath/$asset") //Open file on device
            if (!assetDevicePath.exists()) //if file not exist
            {
                val isFolder = assetManager.list("$assetPath/$asset") //Get list of asset from assets/assetPath/asset/
                if (isFolder?.size != 0) //asset is a folder
                {
                    Log.d("AAR","$assetPath/$asset is a folder")
                    assetDevicePath.mkdirs() //We create the file
                    _mvAssetFolder(context, RootDevicePath, "$assetPath/$asset", "$devicePath/$asset") //We move file in the folder
                }
                else //asset is a file
                {
                    Log.d("AAR","$assetPath/$asset is a file")
                    //We move file
                    assetManager.open("$assetPath/$asset").use { input ->
                        FileOutputStream(File(RootDevicePath, "$devicePath/$asset")).use { output ->
                            var len: Int
                            while (input.read(buffer).also { len = it } > 0)
                                output.write(buffer, 0, len)
                            output.flush()
                        }
                    }
                }
            }
            else
                Log.d("AAR","the file or folder ${RootDevicePath.absolutePath}/$devicePath/$asset already exist")
        }
    }
}
