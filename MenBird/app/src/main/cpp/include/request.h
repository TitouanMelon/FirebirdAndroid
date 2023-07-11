//
// Created by Melon Titouan on 12/06/2023.
//

#ifndef DEF_REQUEST
#define DEF_REQUEST

#include <jni.h>
#include <string>
#include "firebird/Interface.h"
#include "firebird/Message.h"

namespace Firebird
{
    #define FB_VERSION 3 ///< Version of firebird use

    /**
     * Structure wich contain metadata of column in table
     */
    typedef struct MyField MyField;
    struct MyField {
        const char *name; ///< Column's name
        const char *alias; ///< Column's alias
        const char *printInfo; ///< Describe if API can deal the data or not
        unsigned length; ///< Length of the data in byte
        unsigned offset; ///< Offset beetween buffers start and datas start in byte
        unsigned null; ///< Offset beetween buffers start and data's end in byte if the value is null
        unsigned t; ///< SQL type of the data
        unsigned sub; ///< SQL sub type of the data
        int scale; ///< decimal shift for fixed decimal
    };

    /**
     * Structure wich contains info on database
     */
    typedef struct DbInfo DbInfo;
    struct DbInfo{
        char *dbPath; ///< Path of database
        char *fbVersion; ///< version of database
        char *dbUser; ///< user use to connect to the database
        int nbReads; ///< number of reads in database
        int nbWrites; ///< number of writes in database
        int nbFetches; ///< number of fetches in database
        int nbMarks; ///< number of marks in database
        int pageSize; ///< pageSize of database
        int pageBuffer; ///< pageBuffers of database
        std::byte dialect; ///< dialect use in database
    };

    /**
     * Contains function to execute SQL request in firebird database
     */
    class Request {
        public:
            /**
             * Init the parameters and get the database's info in the DBinfo struct
             * @param env - java env
             * @param master - entry point of libfbclient
             * @param util - class for function to encode and decode data
             * @param statusWrapper - To catch exception
             * @param attachment - Database
             */
            Request(JNIEnv* env, IMaster* master, IUtil* util, std::unique_ptr<ThrowStatusWrapper> &statusWrapper, IAttachment* attachment) :
                    _env(env), _master(master), _util(util), _statusWrapper(statusWrapper), _attachment(attachment), _inData(nullptr), _outData(nullptr),
                    _nbOutputLine(0), _nbOutputCols(0), _dbInfo({}){_GetInfoDb();}

            /**
             * Execute request, set _nbOutputLine, _nbOutputCols and fill the _outData array with result data.
             * @param req - String wich represent the SQL request
             */
            void executeReq(const std::string& req);

            /**
             * Convert jobjectArray to std::string[] in _inData
             * @param inJavaData - Java Array of input data
             */
            void InJavaToC(const jobjectArray& inJavaData);

            /**
             * Convert _outData in java array of array of string
             * @return Array<Array<Array<String>>> - result data in array wich correspond at [line][cols]{Data | Type} where when line = 0 is header information
             */
            jobjectArray OutCToJava();

            /**
             * Convert jobjectArray to std::string[] in _inData then execute request and return result data in java array format
             * @param req - String wich represent the SQL request
             * @param inJavaData - Java Array of input data
             * @return Array<Array<Array<String>>> - result data in array wich correspond at [line][cols]{Data | Type} where when line = 0 is header information
             */
            jobjectArray executeReqAndReturn(const std::string& req, const jobjectArray& inJavaData);

        private:
            /**
             * Fill _dbInfo with informations about the database
             */
            void _GetInfoDb();
            /**
             * Execute request with no result data
             * @param req - String wich represent the SQL request
             */
            void _executeNotSelectReq(const std::string& req);

            /**
             * Execute request with result data.
             * @param req - String wich represent the SQL request
             */
            void _executeSelectReq(const std::string& req);

            /**
             * Get Output data and insert data value and data type in data array
             * @param tra - sql transaction
             * @param field - column information
             * @param buf - result data buffer
             * @param data - array to store value and type {data.toString, type.toString}
             */
            void _GetOutputData(ITransaction *tra, const MyField& field, const unsigned char *buf, std::string data[2]);

            /**
             * Transform the _inData array in input buffer to use it in sql request in place of '?' character
             * @param tra - sql transaction
             * @param meta - metadata of input detected in sql request
             * @param builder - builder to create input metadata
             * @param buffer - buffer to fill with input data
             * @param cols - number of input
             */
            void _GetInputData(ITransaction *tra, IMessageMetadata *meta, IMetadataBuilder *builder, unsigned char *buffer, const int cols);

            JNIEnv* _env; ///< env java pour avoir les fonctions de conversions JNI
            IMaster* _master; ///< entryPoint of libfbclient
            IUtil* _util; ///< function to encode and decode data
            std::unique_ptr<ThrowStatusWrapper> &_statusWrapper; ///< To catch exception
            IAttachment* _attachment; ///< Database
            std::string *_inData; ///< //Tableau des paramètres d'entrés
            std::string ***_outData; ///< //Tableau des paramètres de sortie sous la forme [Line][Colonne]{Data, TypeData}
            int _nbOutputLine; ///< //Nombre de lignes
            int _nbOutputCols; ///< //Nombre de colonnes
            DbInfo _dbInfo; ///< Information about database
    };
}

#endif //DEF_REQUEST