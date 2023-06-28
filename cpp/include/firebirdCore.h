//
// Created by Titouan Melon on 26/06/2023.
//

#ifndef DEF_FIREBIRDCORE
#define DEF_FIREBIRDCORE

#include <dlfcn.h>
#include "request.h"

namespace Firebird
{
    /**
     * Convert java string to std::string
     * @param env - java env
     * @param str - java string
     * @return std::string
     */
    static std::string convertJString(JNIEnv* env, jstring str);

    /**
     * Convert c error in java error and throw it
     * @param env - java env
     */
    static void jniRethrow(JNIEnv* env);

    /**
     * Contains core functions to init the API and connect to database
     */
    class Core {
        public:
            Request *requestFunction = nullptr; ///< contain the function to execute request in database

            /**
             * Construct empty class
             */
            Core() {}

            /**
             * Construct class with java environnement set
             * @param env - java virtual environnement
             */
            Core(JNIEnv* env) {_env = env;}

            /**
             * Get Iutil interface
             * @return _util - Iutil interface use to encode and decode data
             */
            auto getUtil(){return _util;}

            /**
             * Connect to a database and init the Request class
             * @param databaseName - Name of the database
             * @param username - username
             * @param password - password
             */
            void connect(const std::string& databaseName, const std::string& username, const std::string& password)
            {
                _loadLibrary();

                IXpbBuilder* dpb = _util->getXpbBuilder(_statusWrapper.get(), IXpbBuilder::DPB, nullptr, 0);
                dpb->insertInt(_statusWrapper.get(), isc_dpb_page_size, 4 * 1024);
                dpb->insertString(_statusWrapper.get(), isc_dpb_user_name, username.c_str());
                dpb->insertString(_statusWrapper.get(), isc_dpb_password, password.c_str());

                try
                {
                    _attachment = _dispatcher->attachDatabase(
                            _statusWrapper.get(),
                            databaseName.c_str(),
                            dpb->getBufferLength(_statusWrapper.get()),
                            dpb->getBuffer(_statusWrapper.get()));
                }
                catch (const FbException& e)
                {
                    _attachment = _dispatcher->createDatabase(
                            _statusWrapper.get(),
                            databaseName.c_str(),
                            dpb->getBufferLength(_statusWrapper.get()),
                            dpb->getBuffer(_statusWrapper.get()));
                }

                requestFunction = new Request(_env, _master, _util, _statusWrapper, _attachment);
            }

            /**
             * Disconnect the database and free the Request class
             */
            void disconnect()
            {
                if (_attachment)
                {
                    _attachment->detach(_statusWrapper.get());
                    _attachment->release();
                    _attachment = nullptr;
                }

                free(requestFunction);

                _unloadLibrary();
            }

        private:
            /**
             * Load the libfbclient library
             */
            void _loadLibrary()
            {
                if (_handle)
                    return;

                if (!(_handle = dlopen(LIB_FBCLIENT, RTLD_NOW)))
                    throw std::runtime_error("Error loading Firebird client library.");

                if (!(_masterFunc = (_GetMasterPtr) dlsym(_handle, SYMBOL_GET_MASTER_INTERFACE)))
                {
                    dlclose(_handle);
                    _handle = nullptr;
                    throw std::runtime_error("Error getting Firebird master interface.");
                }

                _master = _masterFunc();
                _util = _master->getUtilInterface();
                _dispatcher = _master->getDispatcher();
                _status = _master->getStatus();
                _statusWrapper = std::make_unique<ThrowStatusWrapper>(_status);
            }

            /**
             * Unload the libfbclient library
             */
            void _unloadLibrary()
            {
                if (_handle)
                {
                    _dispatcher->shutdown(_statusWrapper.get(), 0, fb_shutrsn_app_stopped);
                    _status->dispose();
                    _dispatcher->release();
                    dlclose(_handle);
                    _handle = nullptr;
                }
            }

            using _GetMasterPtr = decltype(&fb_get_master_interface);

            static constexpr auto LIB_FBCLIENT = "libfbclient.so"; ///< Name of libfbclient file
            static constexpr auto SYMBOL_GET_MASTER_INTERFACE = "fb_get_master_interface"; ///< entry point of main function in libfbclient

            void* _handle = nullptr; ///< Use to load the libfbclient library
            _GetMasterPtr _masterFunc = nullptr; ///< Use to load the main function of libfbclient
            IMaster* _master = nullptr; ///< entryPoint of libfbclient
            IUtil* _util = nullptr; ///< function to encode and decode data
            IProvider* _dispatcher = nullptr; ///< Use to unload library
            IStatus* _status = nullptr; ///< Use to create _statusWrapper ptr
            std::unique_ptr<ThrowStatusWrapper> _statusWrapper; ///< To catch exception
            IAttachment* _attachment = nullptr; ///< Database
            JNIEnv* _env = nullptr; ///< env java pour avoir les fonctions de conversions JNI
    };
}

extern Firebird::Core core; ///< Use to allow the possibilities to declare Core class in other file that in firebirdCore.cpp

#endif //DEF_FIREBIRDCORE
