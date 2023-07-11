//
// Created by Titouan Melon on 26/06/2023.
//

#include "firebirdCore.h"

/**
 * This two line can be put in any cpp's file if you include firebirdCore.h in this file
 */
Firebird::Core core; ///< Link to the extern declaration in firebirdCore.h
extern "C" JNIEXPORT void JNICALL Java_com_example_menbird_Firebird_initAPI(JNIEnv *env, jobject self){core = Firebird::Core(env);} ///< Init the Core class

namespace Firebird {
    // Converts JNI string to std::string.
    static std::string convertJString(JNIEnv* env, jstring str)
    {
        if (!str)
            return {};

        const auto len = env->GetStringUTFLength(str);
        const auto strChars = env->GetStringUTFChars(str, nullptr);

        std::string result(strChars, len);

        env->ReleaseStringUTFChars(str, strChars);

        return result;
    }

    // Rethrow C++ as JNI exception.
    static void jniRethrow(JNIEnv* env)
    {
        std::string message;

        try {
            throw;
            assert(false);
            return;
        }
        catch (const FbException& e) {
            char buffer[1024];
            core.getUtil()->formatStatus(buffer, sizeof(buffer), e.getStatus());
            message = buffer;
        }
        catch (const std::exception& e) {
            message = e.what();
        }
        catch (...) {
            message = "Unrecognized C++ exception";
        }

        const auto exception = env->FindClass("java/lang/Exception");
        env->ThrowNew(exception, message.c_str());
    }

    extern "C" JNIEXPORT jint JNICALL Java_com_example_menbird_Firebird_getApiVersion(JNIEnv* env, jobject self) {return FB_VERSION;} ///< @return version of api

    /**
     * Connect to database
     * @param env - automatically pass by java
     * @param self - automatically pass by java
     * @param databaseName - absolute path of database file
     * @param user - username
     * @param pass - user's password
     * @throw Firbird::exception
     */
    extern "C" JNIEXPORT void JNICALL Java_com_example_menbird_Firebird_connect(JNIEnv* env, jobject self, jstring databaseName, jstring user, jstring pass)
    {
        try {core.connect(convertJString(env, databaseName), convertJString(env, user), convertJString(env, pass));}
        catch (...) {jniRethrow(env);}
    }

    /**
     * Disconnect database
     * @param env - automatically pass by java
     * @param self - automatically pass by java
     * @throw Firbird::exception
     */
    extern "C" JNIEXPORT void JNICALL Java_com_example_menbird_Firebird_disconnect(JNIEnv* env, jobject self)
    {
        try {core.disconnect();}
        catch (...) {jniRethrow(env);}
    }

    /**
     * Execute request and return the result
     * @param env - automatically pass by java
     * @param self - automatically pass by java
     * @param req - SQL request
     * @param inJavaData - array of input parameters
     * @return java object array
     * @throw Firbird::exception
     */
    extern "C" JNIEXPORT jobjectArray JNICALL Java_com_example_menbird_Firebird_executeReqC(JNIEnv* env, jobject self, jstring req, jobjectArray inJavaData)
    {
        try {
#if 1
            return core.requestFunction->executeReqAndReturn(convertJString(env, req), inJavaData);
#else
            core.requestFunction->InJavaToC(inJavaData);
            core.requestFunction->executeReq(convertJString(env, req));
            jobjectArray out = core.requestFunction->OutCToJava();
            return out;
#endif
        }
        catch (...) {
            jniRethrow(env);
            return nullptr;
        }
    }
}