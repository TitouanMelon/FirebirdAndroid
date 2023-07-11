//
// Created by Melon Titouan on 12/06/2023.
//

#include "request.h"

namespace Firebird
{
    void Request::executeReq(const std::string& req) {
        try {
            if (req.find("SELECT") != -1 or req.find("select") != -1 or req.find("Select") != -1) //The request want have result data
                _executeSelectReq(req);
            else //Request don't have result data
            {
                _nbOutputCols = -1;
                _nbOutputLine = -1;
                _outData = nullptr;
                _executeNotSelectReq(req);
            }
        }
        catch (...) {
            throw;
        }
    }

    jobjectArray Request::executeReqAndReturn(const std::string& req, const jobjectArray& inJavaData)
    {
        try {
            InJavaToC(inJavaData); //Convert java array to std::string array
            executeReq(req); //Execute request
            return OutCToJava(); //Convert C array to java array and return it
        } catch (...) {
            throw ;
        }
    }

    void Request::_executeNotSelectReq(const std::string& req) {
        //Ouvre une transaction
        const auto transaction = _attachment->startTransaction(_statusWrapper.get(), 0, nullptr);

        try {
            //Ouvre un curseur pour préparer la requete
            IStatement *stmt = _attachment->prepare(_statusWrapper.get(),
                                                             transaction,
                                                             0,
                                                             req.c_str(),
                                                             SQL_DIALECT_CURRENT,
                                                             IStatement::PREPARE_PREFETCH_METADATA);

            //Recupere les infos des paramètres d'entrées (? dans la requete)
            IMessageMetadata *metaInput = stmt->getInputMetadata(_statusWrapper.get());

            int nbInput = (int)metaInput->getCount(_statusWrapper.get()); //Nombre de parametre en entrées
            auto *bufferInput = new unsigned char[metaInput->getMessageLength(_statusWrapper.get())]; //Buffer pour acceuillir les paramétres en entrées

            if (nbInput != 0 && _inData != nullptr) //On a des paramètres en entrées
            {
                IMetadataBuilder *builder = _master->getMetadataBuilder(_statusWrapper.get(),nbInput); //Builder pour construire les metadata des input
                _GetInputData(transaction, metaInput, builder, bufferInput, nbInput); //Recupere les parametres d'entrés et les insere dans le buffer
                metaInput = builder->getMetadata(_statusWrapper.get()); //Recupere les metadata créées
            }
            else //On a aucun paramètre
            {
                metaInput = nullptr;
                bufferInput = nullptr;
            }

            //Execute la requete
            stmt->execute(
                    _statusWrapper.get(),
                    transaction,
                    metaInput,
                    bufferInput,
                    nullptr,
                    nullptr);

            transaction->commit(_statusWrapper.get());
            transaction->release();

            return;
        }
        catch (const FbException &error) {
            // handle error
            fflush(stdout);

            transaction->rollback(_statusWrapper.get());

            char buf[256];
            _master->getUtilInterface()->formatStatus(buf, sizeof(buf), error.getStatus());
            throw error;
        }
    }

    void Request::_executeSelectReq(const std::string& req) {
        _outData = nullptr;
        _nbOutputLine = 0;
        _nbOutputCols = 0;

        //Ouvre une transaction
        const auto transaction = _attachment->startTransaction(_statusWrapper.get(), 0, nullptr);

        try {
            //Ouvre un curseur pour preparer la requete
            IStatement *stmt = _attachment->prepare(_statusWrapper.get(),
                                                             transaction,
                                                             0,
                                                             req.c_str(),
                                                             SQL_DIALECT_CURRENT,
                                                             IStatement::PREPARE_PREFETCH_METADATA);

            //Recupere les infos des paramètres d'entrées (? dans la requete)
            IMessageMetadata *metaInput = stmt->getInputMetadata(_statusWrapper.get());
            int nbInput = (int)metaInput->getCount(_statusWrapper.get()); //Nombre de parametre en entrées
            int len = (int)metaInput->getMessageLength(_statusWrapper.get());
            auto *bufferInput = new unsigned char[len]; //Buffer pour acceuillir les paramétres en entrées

            if (nbInput != 0  && _inData != nullptr)
            {
                IMetadataBuilder *builder = _master->getMetadataBuilder(_statusWrapper.get(), nbInput); //Builder pour construire les metadata des input
                _GetInputData(transaction, metaInput, builder, bufferInput, nbInput); //Recupere les parametres d'entrés et les insere dans le buffer
                metaInput = builder->getMetadata(_statusWrapper.get()); //Recupere les metadata créées
            }
            else
            {
                metaInput = nullptr;
                bufferInput = nullptr;
            }

            // get list of columns
            IMessageMetadata *metaOutput = stmt->getOutputMetadata(_statusWrapper.get());
            // open cursor
            IResultSet *curs = stmt->openCursor(_statusWrapper.get(), transaction,
                                                          metaInput, bufferInput, nullptr, 0);

            _nbOutputCols = (int)metaOutput->getCount(_statusWrapper.get()); //Nombre de colonne dans le resultat

            auto *fields = new MyField[_nbOutputCols];
            memset(fields, 0, sizeof(MyField) * _nbOutputCols);

            // parse columns list & coerce datatype(s)
            for (unsigned j = 0; j < _nbOutputCols; ++j) {
                fields[j].t = metaOutput->getType(_statusWrapper.get(), j) & ~1;
                fields[j].sub = metaOutput->getSubType(_statusWrapper.get(), j);
                fields[j].printInfo = "True";

                switch (fields[j].t) {
                    case SQL_ARRAY:
                    case SQL_BLOB:
                    case SQL_TEXT:
                    case SQL_VARYING:
                    case SQL_SHORT:
                    case SQL_LONG:
                    case SQL_INT64:
                    case SQL_FLOAT :
                    case SQL_DOUBLE:
                    case SQL_BOOLEAN:
                    case SQL_TIMESTAMP:
                    case SQL_TYPE_DATE:
                    case SQL_TYPE_TIME:
    #if FB_VERSION > 3
                    case SQL_TIMESTAMP_TZ:
    #endif
                        break;
                    default:
                        fields[j].printInfo = nullptr;
                        break;
                }

                if (fields[j].printInfo) {
                    fields[j].alias = metaOutput->getAlias(_statusWrapper.get(), j);
                    fields[j].name = metaOutput->getField(_statusWrapper.get(), j);
                    fields[j].length = metaOutput->getLength(_statusWrapper.get(), j);
                    fields[j].offset = metaOutput->getOffset(_statusWrapper.get(), j);
                    fields[j].null = metaOutput->getNullOffset(_statusWrapper.get(), j);
                    fields[j].scale = metaOutput->getScale(_statusWrapper.get(), j);
                }
            }

            // allocate output buffer
            unsigned l = metaOutput->getMessageLength(_statusWrapper.get()); //Taille du buffer de sortie
            auto *bufferOutput = new unsigned char[l]; //Buffer de sortie

            _outData = (std::string ***) malloc(sizeof(std::string **) * 1); //Création de tableau qui contiendra toutes les lignes
            _outData[0] = (std::string **) malloc(sizeof(std::string *) * _nbOutputCols); //Création de la ligne header
            for (unsigned j = 0; j < _nbOutputCols; ++j) {
                _outData[0][j] = new std::string[2];
                if (fields[j].name != nullptr)
                    _outData[0][j][0] = fields[j].alias;
                else
                    _outData[0][j][0] = "Null";
                _outData[0][j][1] = "SQL_HEADER";
            }

            while (curs->fetchNext(_statusWrapper.get(), bufferOutput) == IStatus::RESULT_OK) //Tant qu'il y a une ligne de donnée
            {
                _nbOutputLine++; //On incrémente le nombre de ligne
                _outData = (std::string ***) realloc(_outData, sizeof(std::string **) * (_nbOutputLine + 1)); //On ajoute une ligne dans le tableau de données de sortie
                _outData[_nbOutputLine] = (std::string **) malloc(sizeof(std::string *) * _nbOutputCols); //On alloue la ligne

                for (unsigned j = 0; j < _nbOutputCols; ++j) {
                    _outData[_nbOutputLine][j] = new std::string[2]; //On vient creer les champs contenant la données et son type 0:data, 1:type
                    // call field's function to print it
                    _GetOutputData(transaction, fields[j], bufferOutput, _outData[_nbOutputLine][j]);
                }
            }

            // release automatically created metadata
            // metadata is not database object, therefore no specific call to close it
            metaOutput->release();
            metaOutput = nullptr;

            // close interfaces
            curs->close(_statusWrapper.get());
            curs = nullptr;

            stmt->free(_statusWrapper.get());
            stmt = nullptr;

            transaction->commit(_statusWrapper.get());
            transaction->release();

            _nbOutputLine += 1; //On ajoute 1 pour prendre en compte la ligne header

            return;
        }
        catch (const FbException &error) {
            // handle error
            fflush(stdout);

            transaction->rollback(_statusWrapper.get());

            char buf[256];
            _master->getUtilInterface()->formatStatus(buf, sizeof(buf), error.getStatus());
            throw error;
        }
    }

    void Request::_GetOutputData(ITransaction *tra, const MyField& field, const unsigned char *buf, std::string data[2]) {
        data[1] = std::to_string(field.t); //Type de la donnée

        if (*((short *) (buf + field.null))) //Si la valeur est null
        {
            data[0] = "Null";
            data[1] = "-1";
            return;
        }

        // IBlob makes it possible to read/write BLOB data
        IBlob *blob = nullptr;

        double *dval = nullptr;
        float *fval = nullptr;
        int *ival = nullptr;
        long int *lint = nullptr;
        short int *sint = nullptr;
        bool *bval = nullptr;
        char *str = nullptr;
        int size = 0;

    #if FB_VERSION > 3
        ISC_TIMESTAMP_TZ *isc_timestamp_tz = nullptr;
    #endif
        ISC_TIMESTAMP *isc_timestamp = nullptr;
        ISC_TIME *isc_time = nullptr;
        ISC_DATE *isc_date = nullptr;

        struct {
            unsigned int day = 0;
            unsigned int month = 0;
            unsigned int year = 0;
            unsigned int fraction = 0;
            unsigned int second = 0;
            unsigned int minutes = 0;
            unsigned int hours = 0;
            unsigned int timeZone = 3;
            char charTimeZone[4] = {};
        } time;
        char date_s[40] = "";

        switch (field.t) {
            // text fields
            case SQL_TEXT:
                str = (char *) malloc(sizeof(char) * field.length+1); //We alloc length + 1 to take count of '\0' characters
                memcpy(str, buf + field.offset, field.length);
                str[field.length] = '\0'; //We add the null character because SQL_TEXT don't have this in firebird
                data[0] = str;
                free(str);
                break;
            case SQL_VARYING:
                str = (char *) malloc(sizeof(char) * field.length+1);
                memcpy(str, buf + field.offset + sizeof(short), field.length); //we add sizeof(short) because at the beginning of varying we have the len of the string
                str[field.length] = '\0';
                data[0] = str;
                free(str);
                break;
            // numeric fields
            case SQL_SHORT:
                sint = (short int *) (buf + field.offset);
                if (field.scale == 0)
                    data[0] = std::to_string(*sint);
                else
                {
                    data[0] = std::to_string((double)(*sint * pow(10, field.scale)));
                    data[1] = std::to_string(SQL_DOUBLE);
                }

                break;
            case SQL_LONG:
                ival = (int *) (buf + field.offset);
                if (field.scale == 0)
                    data[0] = std::to_string(*ival);
                else
                {
                    data[0] = std::to_string((double)(*ival * pow(10, field.scale)));
                    data[1] = std::to_string(SQL_DOUBLE);
                }

                break;
            case SQL_INT64:
                lint = (long int *) (buf + field.offset);
                if (field.scale == 0)
                    data[0] = std::to_string(*lint);
                else
                {
                    data[0] = std::to_string((double)(*lint * pow(10, field.scale)));
                    data[1] = std::to_string(SQL_DOUBLE);
                }
                break;
            case SQL_FLOAT :
                fval = (float *) (buf + field.offset);
                data[0] = std::to_string(*fval);
                break;
            case SQL_DOUBLE:
                dval = (double *) (buf + field.offset);
                data[0] = std::to_string(*dval);
                break;
            case SQL_BOOLEAN:
                bval = (bool *) (buf + field.offset);
                if (*bval)
                    data[0] = "true";
                else
                    data[0] = "false";
                break;
            // blob requires more handling in DB
            case SQL_ARRAY:
            case SQL_BLOB:
                try {
                    str = (char *) malloc(sizeof(char));
                    // use attachment's method to access BLOB object
                    blob = _attachment->openBlob(_statusWrapper.get(), tra, (ISC_QUAD *) (buf + field.offset), 0,nullptr);

                    char segbuf[16];
                    unsigned len;
                    // read data segment by segment
                    for (;;)
                    {
                        int cc = blob->getSegment(_statusWrapper.get(), sizeof(segbuf), segbuf,&len);
                        if (cc != IStatus::RESULT_OK && cc != IStatus::RESULT_SEGMENT)
                            break;
                        if (size > 255 || len == 0)
                            break;

                        str = (char *) realloc(str, sizeof(char) * (size + len));
                        memcpy(str + size, segbuf, len);
                        size += len;
                        if (len != 16 || size > 255)
                        {
                            str = (char *) realloc(str, sizeof(char) * (size + 1));
                            memcpy(str + size, " ", 1);
                            size += 1;
                        }
                    }

                    str = (char *) realloc(str, sizeof(char) * (size + 1));
                    memcpy(str + size, "\0", 1);

                    // close BLOB after receiving all data
                    blob->close(_statusWrapper.get());
                    blob = nullptr;

                    data[0] = str;
                    free(str);
                }
                catch (...) {
                    if (blob)
                        blob->release();
                    throw;
                }
                break;
    #if FB_VERSION > 3
            case SQL_TIMESTAMP_TZ:
                isc_timestamp_tz = (ISC_TIMESTAMP_TZ *) (buf + field.offset);
                _util->decodeTimeStampTz(statusWrapper.get(), isc_timestamp_tz, &time.year,
                    &time.month, &time.day, &time.hours, &time.minutes,
                    &time.second, &time.fraction, time.timeZone,
                    time.charTimeZone);
                sprintf(date_s, "%02d-%02d-%04d %02d:%02d:%02d.%04d %03s",
                    time.day,
                    time.month,
                    time.year,
                    time.hours,
                    time.minutes,
                    time.second,
                    time.fraction,
                    time.charTimeZone);
                data[0] = date_s;
                break;
    #endif
            case SQL_TIMESTAMP:
                isc_timestamp = (ISC_TIMESTAMP *)(buf+field.offset);
    #if FB_VERSION > 3
                isc_timestamp_tz = (ISC_TIMESTAMP_TZ *)malloc(sizeof (isc_timestamp_tz));
                isc_timestamp_tz->utc_timestamp = *isc_timestamp;
                isc_timestamp_tz->time_zone = 65535;
                _util->decodeTimeStampTz(statusWrapper.get(), isc_timestamp_tz, &time.year, &time.month, &time.day, &time.hours, &time.minutes, &time.second, &time.fraction, time.timeZone, time.charTimeZone);
    #else
                _util->decodeDate(isc_timestamp->timestamp_date, &time.year, &time.month, &time.day);
                _util->decodeTime(isc_timestamp->timestamp_time, &time.hours, &time.minutes, &time.second, &time.fraction);
    #endif
                sprintf(date_s, "%02d-%02d-%04d %02d:%02d:%02d.%04d",
                        time.day,
                        time.month,
                        time.year,
                        time.hours,
                        time.minutes,
                        time.second,
                        time.fraction);
                data[0] = date_s;
    #if FB_VERSION > 3
                free(isc_timestamp_tz);
    #endif
                break;
            case SQL_TYPE_DATE:
                isc_date = (ISC_DATE *)(buf+field.offset);
                _util->decodeDate(*isc_date, &time.year, &time.month, &time.day);
                sprintf(date_s, "%02d-%02d-%04d",
                        time.day,
                        time.month,
                        time.year);
                data[0] = date_s;
                break;
            case SQL_TYPE_TIME:
                isc_time = (ISC_TIME *)(buf+field.offset);
                _util->decodeTime(*isc_time, &time.hours, &time.minutes, &time.second, &time.fraction);
                sprintf(date_s, "%02d:%02d:%02d.%04d",
                        time.hours,
                        time.minutes,
                        time.second,
                        time.fraction);
                data[0] = date_s;
                break;
            // do not plan to have all types here
            default:
                //__android_log_print(ANDROID_LOG_WARN, "UNKNOW", "Unknown type <%u>", type);
                data[0] = "DATA NOT IMPLMENTED";
        }
    }

    void Request::_GetInputData(ITransaction *tra, IMessageMetadata *meta, IMetadataBuilder *builder, unsigned char *buffer, const int cols) {
        // IBlob makes it possible to read/write BLOB data
        IBlob *blob = nullptr;

        short nullFlag = 0;
        double dval = 0;
        float fval = 0;
        int ival = 0;
        long lint = 0;
        short sint = 0;
        bool bval = false;

        ISC_TIME isc_time = 0;
        ISC_DATE isc_date= 0;
    #if FB_VERSION > 3
        ISC_TIMESTAMP_TZ isc_timestamp_tz = {};
        char timeZone[] = "GMT";
    #else
        ISC_TIMESTAMP isc_timestamp = {};
    #endif

        MyField field = {};
        struct tm times = {};
        int index = 0;
        int nb_format = 10;
        std::string timestamp_tz_format[] = {"%Y-%m-%d %H:%M:%S", "%d-%m-%Y %H:%M:%S", "%Y-%m-%dT%H:%M:%S", "%d-%m-%YT%H:%M:%S",
                                             "%Y.%m.%d %H:%M:%S", "%d.%m.%Y %H:%M:%S", "%Y.%m.%dT%H:%M:%S", "%d.%m.%YT%H:%M:%S",
                                             "%Y/%m/%d %H:%M:%S", "%d/%m/%Y %H:%M:%S","%Y/%m/%dT%H:%M:%S","%d/%m/%YT%H:%M:%S"};
        std::string  date_format[] = {"%Y-%m-%d", "%d-%m-%Y",
                                      "%Y.%m.%d", "%d.%m.%Y",
                                      "%Y/%m/%d", "%d/%m/%Y"};
        std::string dateStr = std::string("");
        char *tmp = nullptr;


        for (int i = 0; i < cols; i++) {
            field = {};
            times = {};

            field.t = (int)meta->getType(_statusWrapper.get(), i) & ~1;
            field.sub = (int)meta->getSubType(_statusWrapper.get(), i);
            field.length = (int)meta->getLength(_statusWrapper.get(), i);
            field.offset = (int)meta->getOffset(_statusWrapper.get(), i);
            field.null = (int)meta->getNullOffset(_statusWrapper.get(), i);
            field.scale = meta->getScale(_statusWrapper.get(), i);
            field.name = meta->getField(_statusWrapper.get(), i);
            field.alias = meta->getAlias(_statusWrapper.get(), i);

            builder->setType(_statusWrapper.get(), i, field.t);
            builder->setSubType(_statusWrapper.get(), i, field.sub);
    #if FB_VERSION > 3
            builder->setField(_statusWrapper.get(), i, field.name);
            builder->setAlias(_statusWrapper.get(), i, field.alias);
    #endif
            builder->setLength(_statusWrapper.get(), i, field.length);
            builder->setScale(_statusWrapper.get(), i, field.scale);

            switch (field.t) {
                // text fields
                case SQL_TEXT:
                    memcpy(buffer + field.offset, _inData[i].c_str(), _inData[i].size());
                    //We fill the string with space to respect the string's length
                    for (int j=(int)_inData[i].size() ; j<field.length ; j++)
                    {
                        memcpy(buffer + field.offset + j, " ", 1);
                    }
                    break;
                case SQL_VARYING:
                    sint = (short) _inData[i].size(); //For varying we add at the beginning the size of string
                    memcpy(buffer + field.offset, &sint, sizeof(short));
                    memcpy(buffer + field.offset + sizeof(short), _inData[i].c_str(), _inData[i].size());
                    break;
                // numeric fields
                case SQL_SHORT:
                    if (field.scale == 0)
                        sint = (short)atoi(_inData[i].c_str());
                    else
                        sint = (short)(atof(_inData[i].c_str())/pow(10, field.scale));
                    memcpy(buffer + field.offset, &sint, field.length);
                    break;
                case SQL_LONG:
                    if (field.scale == 0)
                        ival = atoi(_inData[i].c_str());
                    else
                        ival = (int)(atof(_inData[i].c_str())/pow(10, field.scale));
                    memcpy(buffer + field.offset, &ival, field.length);
                    break;
                case SQL_INT64:
                    if (field.scale == 0)
                        lint = atol(_inData[i].c_str());
                    else
                        lint = (long)(atof(_inData[i].c_str())/pow(10, field.scale));
                    memcpy(buffer + field.offset, &lint, field.length);
                    break;
                case SQL_FLOAT :
                    fval = (float)atof(_inData[i].c_str());
                    memcpy(buffer + field.offset, &fval, field.length);
                    break;
                case SQL_DOUBLE:
                    dval = atof(_inData[i].c_str());
                    memcpy(buffer + field.offset, &dval, field.length);
                    break;
                case SQL_BOOLEAN:
                    if (_inData[i] == "1" || _inData[i] == "true" || _inData[i] == "True")
                        bval = true;
                    else if (_inData[i] == "0" || _inData[i] == "false" || _inData[i] == "False")
                        bval = false;
                    memcpy(buffer + field.offset, &bval, field.length);
                    break;
                // blob requires more handling in DB
                case SQL_ARRAY:
                case SQL_BLOB:
                    // create blob
                    blob = _attachment->createBlob(_statusWrapper.get(), tra, (GDS_QUAD_t *)(buffer + field.offset), 0,nullptr);
                    // populate blob with data
                    blob->putSegment(_statusWrapper.get(), _inData[i].size(), _inData[i].c_str());
                    blob->close(_statusWrapper.get());
                    blob = nullptr;
                    break;
    #if FB_VERSION > 3
                case SQL_TIMESTAMP_TZ:
    #endif
                case SQL_TIMESTAMP:
                    while (dateStr != _inData[i].substr(0, strlen("01.01.0000T00:00:00 "))) //Cherche le bon format de date
                    {
                        if (index == nb_format)
                            throw;
                        strptime(_inData[i].c_str(), timestamp_tz_format[index].c_str(), &times);
                        tmp = (char*) malloc(sizeof (char)*strlen("01.01.0000T00:00:00 "));
                        strftime(tmp, strlen("01.01.0000T00:00:00 "), timestamp_tz_format[index].c_str(), &times);
                        dateStr = tmp;
                        index++;
                    }
    #if FB_VERSION > 3
                    _util->encodeTimeStampTz(statusWrapper.get(), &isc_timestamp_tz, times.tm_year+1900, times.tm_mon+1, times.tm_mday, times.tm_hour, times.tm_min, times.tm_sec, 0, timeZone);
                    if (field.t == SQL_TIMESTAMP)
                    {
                        memcpy(buffer + field..offset, &isc_timestamp_tz.utc_timestamp, field.length);
                    }
                    else if (field.t == SQL_TIMESTAMP_TZ)
                    {
                        memcpy(buffer + field.offset, &isc_timestamp_tz, length);
                    }
    #else
                    isc_timestamp.timestamp_date = _util->encodeDate(times.tm_year+1900, times.tm_mon+1, times.tm_mday);
                    isc_timestamp.timestamp_time = _util->encodeTime(times.tm_hour, times.tm_min, times.tm_sec, 0);
                    memcpy(buffer + field.offset, &isc_timestamp, field.length);
    #endif
                    break;
                case SQL_TYPE_DATE:
                    while (dateStr != _inData[i].substr(0, strlen("01.01.0000 "))) //Cherche le bon format de date
                    {
                        if (index == (nb_format/2))
                            throw;
                        strptime(_inData[i].c_str(), date_format[index].c_str(), &times);
                        tmp = (char*) malloc(sizeof (char)*strlen("01.01.0000 "));
                        strftime(tmp, strlen("01.01.0000 "), date_format[index].c_str(), &times);
                        dateStr = tmp;
                        index++;
                    }
                    isc_date = _util->encodeDate(times.tm_year+1900, times.tm_mon+1, times.tm_mday);
                    memcpy(buffer + field.offset, &isc_date, field.length);
                    break;
                case SQL_TYPE_TIME:
                    tmp = strptime(_inData[i].c_str(), "%H:%M:%S", &times);
                    if (tmp == nullptr)
                        throw;
                    isc_time = _util->encodeTime(times.tm_hour, times.tm_min, times.tm_sec, 0);
                    memcpy(buffer + field.offset, &isc_time, field.length);
                    break;
                // do not plan to have all types here
                default:
                    break;
            }

            memcpy(buffer + field.null, &nullFlag, sizeof(short));
        }
    }

    void Request::InJavaToC(const jobjectArray& inJavaData)
    {
        int len = _env->GetArrayLength(inJavaData);
        _inData = new std::string[len];

        for (int i=0 ; i<len ; i++) //Pour chaque element du tableau java le transforme en String c++ et l'ajoute au tableau inData
        {
            char* val = (char *)_env->GetStringUTFChars((jstring)(_env->GetObjectArrayElement(inJavaData, i)), nullptr);
            _inData[i] = val;
            free(val);
        }
    }

    jobjectArray Request::OutCToJava()
    {
        try
        {
            if (_outData == nullptr)
                return nullptr;

            //std::string[nbLine(Headers+dataLine)][nbCol][data|type] to jni
            jclass ArrayOfArrayOfString = _env->FindClass("[[Ljava/lang/String;");
            jclass ArrayOfString = _env->FindClass("[Ljava/lang/String;");
            jclass String = _env->FindClass("java/lang/String");

            jint sizeAAAS = _nbOutputLine;

            jobjectArray AAAS = _env->NewObjectArray(sizeAAAS, ArrayOfArrayOfString, nullptr);
            for (jint x = 0; x < sizeAAAS; x++)
            {
                jint sizeAAS = _nbOutputCols;
                jobjectArray AAS = _env->NewObjectArray(sizeAAS, ArrayOfString, nullptr);
                for (jint y = 0; y < sizeAAS; y++)
                {
                    jint sizeAS = 2;
                    jobjectArray AS = _env->NewObjectArray(sizeAS, String, nullptr);
                    for (jint z = 0; z < sizeAS; z++)
                    {
                        int len = 1;
                        char *outStr = (char *)_outData[x][y][z].c_str();
                        std::string str = std::string("");
                        for (int i=0 ; i<_outData[x][y][z].size()+1 ; i+=len) //verifie que des caractère non-UTF8 ne sont pas présent pour eviter un plantage de l'appli
                        {
                            if ((outStr[i] & 0x80) == 0x00) //1 octets
                                len = 1;
                            else if ((outStr[i] & 0xe0) == 0xc0 && (outStr[i+1] & 0xc0) == 0x80) //2 octets
                                len = 2;
                            else if ((outStr[i] & 0xf0) == 0xe0 && (outStr[i+1] & 0xc0) == 0x80 &&
                                     (outStr[i+2] & 0xc0) == 0x80) //3 octets
                                len = 3;
                            else if ((outStr[i] & 0xf8) == 0xf0 && (outStr[i+1] & 0xc0) == 0x80 &&
                                (outStr[i+2] & 0xc0) == 0x80 && (outStr[i+3] & 0xc0) == 0x80) //4 octets
                                len = 4;
                            else { //Non UTF8
                                len = 1;
                                outStr[i] = '?';
                            }

                            for(int j=0 ; j<len ; j++) {
                                str.append(1, outStr[i+j]);
                            }
                        }
                        str.append("\0");
                        _env->SetObjectArrayElement(AS, z, _env->NewStringUTF(str.c_str()));
                    }
                    _env->SetObjectArrayElement(AAS, y, AS);
                }
                _env->SetObjectArrayElement(AAAS, x, AAS);
            }

            free(_outData);

            return AAAS;
        }
        catch (...)
        {
            throw ;
        }
    }

    void Request::_GetInfoDb(){
        _dbInfo = {};

        unsigned char items[] = {
                isc_info_db_id,
                isc_info_reads,
                isc_info_writes,
                isc_info_fetches,
                isc_info_marks,
                isc_info_isc_version,
                isc_info_page_size,
                isc_info_num_buffers,
                isc_info_user_names,
                isc_info_db_sql_dialect,
                isc_info_end};

        unsigned char res_buffer[2048], *p, item;
        short *length;
        _attachment->getInfo(_statusWrapper.get(), sizeof (items), items, sizeof(res_buffer),res_buffer);

        /* Extract the values returned in the result buffer. */
        for (p = res_buffer; *p != isc_info_end ;) {
            item = *p++;
            length = (short*)p;
            p += 2;
            switch (item) {
                case isc_info_db_id:
                    _dbInfo.dbPath = (char *) malloc(sizeof (char) * (*length));
                    memcpy(_dbInfo.dbPath, p, *length);
                    break;
                case isc_info_reads:
                    _dbInfo.nbReads = *((int *)p);
                    break;
                case isc_info_writes:
                    _dbInfo.nbWrites = *((int *)p);
                    break;
                case isc_info_fetches:
                    _dbInfo.nbFetches = *((int *)p);
                    break;
                case isc_info_marks:
                    _dbInfo.nbMarks = *((int *)p);
                    break;
                case isc_info_isc_version:
                    _dbInfo.fbVersion = (char *) malloc(sizeof (char) * (*length));
                    memcpy(_dbInfo.fbVersion, p, *length);
                    break;
                case isc_info_page_size:
                    _dbInfo.pageSize = *((int *)p);
                    break;
                case isc_info_num_buffers:
                    _dbInfo.pageBuffer = *((int *)p);
                    break;
                case isc_info_user_names:
                    _dbInfo.dbUser = (char *) malloc(sizeof (char) * (*length));
                    memcpy(_dbInfo.dbUser, p, *length);
                    break;
                case isc_info_db_sql_dialect:
                    _dbInfo.dialect = *((std::byte *)p);
                    break;
            }
            p += *length;
        }
    }
}