package com.example.menbird

import android.content.ContextWrapper
import android.util.Log
import java.io.File
import org.firebirdsql.android.embedded.FirebirdConf

/**
 * Class which allow communicate with c++ api
 * @property _context - context to get path of user directory
 * @property _inputData - array to store input data
 * @property _outputData - array of map to store output data
 * @property _nbOutputLine - number of line in output data
 * @property _nbOutputCol - number of column in output data line
 * @property SQL_TYPE - enumeration of sql_type
 */
class Firebird(baseContext : ContextWrapper, useUsersFolders : Boolean = true) {
    private var _context : ContextWrapper = baseContext
    private val _useUsersFolders : Boolean = useUsersFolders
    private var _inputData : Array<Any> = emptyArray()
    private var _outputData : Array<MutableMap<String, Any>> = emptyArray()
    private var _nbOutputLine : Int = 0
    private var _nbOutputCol : Int = 0
    private var _SQL_TYPE : Map<String, Int> = mapOf("MY_NULL" to -1, "SQL_TEXT" to 452, "SQL_VARYING" to 448,
        "SQL_SHORT" to 500, "SQL_LONG" to 496, "SQL_FLOAT" to 482, "SQL_DOUBLE" to 480, "SQL_D_FLOAT" to 530,
        "SQL_TIMESTAMP" to 510, "SQL_BLOB" to 520, "SQL_ARRAY" to 540, "SQL_QUAD" to 550, "SQL_TYPE_TIME" to 560,
        "SQL_TYPE_DATE" to 570, "SQL_INT64" to 580, "SQL_TIMESTAMP_TZ_EX" to 32748, "SQL_TIME_TZ_EX" to 32750,
        "SQL_INT128" to 32752, "SQL_TIMESTAMP_TZ" to 32754, "SQL_TIME_TZ" to 32756, "SQL_DEC16" to 32760,
        "SQL_DEC34" to 32762, "SQL_BOOLEAN" to 32764, "SQL_NULL" to 32766
    )

    private external fun initAPI() /** Init the core function with java environnement */
    private external fun connect(databaseName: String, user : String, pass : String) /** Connect to database */
    private external fun disconnect() /** Disconnect database */
    private external fun executeReqC(req: String, inData : Array<Any>): Array<Array<Array<String>>> /** Execute request and return output data */
    private external fun getApiVersion() : Int /** return api version */

    /**
     * Extract the asset and the database from the .aar, load the api library and init it
     */
    init {
        //Use to extract asset and database of the .aar in app's folder on application startup if they not exist
        FirebirdConf.extractAssets(baseContext, useUsersFolders, false)
        FirebirdConf.setEnv(baseContext)
        // Used to load the 'firebirdandroidcpp' library on application startup.
        System.loadLibrary("menbird")
        initAPI()
    }

    /**
     * Get the version of database and API
     * @return String with both version and explication depending difference beetween version
     */
    fun getVersion() : Array<String> {
        val dbVersion : String =  executeReq("SELECT rdb\$get_context('SYSTEM', 'ENGINE_VERSION') as version from rdb\$database", emptyArray())[0]["VERSION"].toString()
        val apiVersion : Int = getApiVersion()

        val dbVersionMaj : Int = dbVersion[0].toString().toInt()

        return if (dbVersionMaj == apiVersion)
            arrayOf("Db : $dbVersion | Api : $apiVersion","Version is correct")
        else if (dbVersionMaj < apiVersion)
            arrayOf("Db : $dbVersion | Api : $apiVersion", "Some requests can crashed the application")
        else //dbVersionMaj > apiVersion
            arrayOf("Db : $dbVersion | Api : $apiVersion", "Some type can be ignored (ex: timestamp with timezone)")
    }

    /**
     * Get all table name in the database and store it in _outputData
     */
    fun getTableName() {
        executeReq("SELECT RDB\$RELATION_NAME as Table_name FROM RDB\$RELATIONS WHERE COALESCE(RDB\$SYSTEM_FLAG, 0) = 0 AND RDB\$RELATION_TYPE = 0")
    }

    /**
     * Get all procdeure name in the database and store it in _outputData
     */
    fun getProcedureName() {
        executeReq("select RDB\$PROCEDURE_NAME as Procedure_name from RDB\$PROCEDURES")
    }

    /**
     * Get all udf name in the database and store it in _outputData
     */
    fun getUDFName() {
        executeReq("SELECT r.RDB\$FUNCTION_NAME as function_name FROM RDB\$FUNCTIONS r")
    }

    /**
     * Get all info of table and store it in _outputData
     * @param tableName - name of the table we want info on
     */
    fun getInfoOnTable(tableName : String) {
        val query = "select a.RDB\$FIELD_NAME as Name, a.RDB\$FIELD_SOURCE as domain_name, case b.RDB\$FIELD_TYPE"+
            "\nwhen 7 then 'SMALLINT'"+
            "\nwhen 8 then 'INTEGER'"+
            "\nwhen 10 then 'FLOAT'"+
            "\nwhen 12 then 'DATE'"+
            "\nwhen 13 then 'TIME'"+
            "\nwhen 14 then 'CHAR'"+
            "\nwhen 16 then 'BIGINT'"+
            "\nwhen 27 then 'DOUBLE PRECISION'"+
            "\nwhen 35 then 'TIMESTAMP'"+
            "\nwhen 37 then 'VARCHAR'"+
            "\nwhen 261 then 'BLOB'"+
        "\nend Type_Name,"+
        "\na.RDB\$FIELD_POSITION as inPosition, b.RDB\$DEFAULT_SOURCE as default_value, b.RDB\$FIELD_SCALE as scale,"+
        "\nb.RDB\$FIELD_SUB_TYPE as subType, b.RDB\$FIELD_PRECISION as NumericLength, b.RDB\$CHARACTER_LENGTH as strLength, a.RDB\$DESCRIPTION as description_1, b.RDB\$DESCRIPTION as description_2"+
        "\nfrom RDB\$RELATION_FIELDS a"+
        "\nleft join RDB\$FIELDS b on b.RDB\$FIELD_NAME = a.rdb\$field_source"+
        "\nwhere a.RDB\$RELATION_NAME = ?"

        executeReq(query, arrayOf(tableName))
        Log.d("GEETABLEINFO", "Info recuperer")
    }

    /**
     * Get all info of procedure and store it in _outputData
     * @param procedureName - name of the procedure we want info on
     */
    fun getInfoOnProcedure(procedureName : String) {
        val query = "select b.rdb\$parameter_name as name_param,"+
        "\nCase b.rdb\$parameter_type"+
        "\nWhen 0 Then 'INPUT'"+
        "\nWhen 1 Then 'OUTPUT'"+
        "\nend type_param,"+
        "\nb.rdb\$parameter_number as position_param,"+
        "\nb.rdb\$field_source as domain_type,"+
        "\ncase c.RDB\$FIELD_TYPE"+
        "\nwhen 7 then 'SMALLINT'"+
        "\nwhen 8 then 'INTEGER'"+
        "\nwhen 10 then 'FLOAT'"+
        "\nwhen 12 then 'DATE'"+
        "\nwhen 13 then 'TIME'"+
        "\nwhen 14 then 'CHAR'"+
        "\nwhen 16 then 'BIGINT'"+
        "\nwhen 27 then 'DOUBLE PRECISION'"+
        "\nwhen 35 then 'TIMESTAMP'"+
        "\nwhen 37 then 'VARCHAR'"+
        "\nwhen 261 then 'BLOB'"+
        "\nend Type_Name,"+
        "\nc.RDB\$DEFAULT_SOURCE as default_value,"+
        "\nc.RDB\$FIELD_SCALE as scale,"+
        "\nc.RDB\$FIELD_SUB_TYPE as subType,"+
        "\nc.RDB\$FIELD_PRECISION as NumericLength,"+
        "\nc.RDB\$CHARACTER_LENGTH as strLength,"+
        "\na.rdb\$description as description_param,"+
        "\nc.RDB\$DESCRIPTION as description_2"+
        "\nfrom RDB\$PROCEDURES a"+
        "\nleft join RDB\$PROCEDURE_PARAMETERS b on a.RDB\$PROCEDURE_NAME = b.RDB\$PROCEDURE_NAME"+
        "\nleft join RDB\$FIELDS c on c.RDB\$FIELD_NAME = b.rdb\$field_source"+
        "\nwhere a.RDB\$PROCEDURE_NAME = ?"

        executeReq(query, arrayOf(procedureName))
    }

    /**
     * Get all info of udf and store it in _outputData
     * @param udfName - name of the udf we want info on
     */
    fun getInfoOnUDF(udfName : String) {
        val query = "Select a.RDB\$MODULE_NAME as module,"+
        "\na.RDB\$ENTRYPOINT as fonction,"+
        "\nb.RDB\$ARGUMENT_POSITION as positionParam,"+
        "\nCase b.RDB\$MECHANISM"+
        "\nWhen 0 Then 'OUTPUT BY VALUE'"+
        "\nWhen 1 Then 'INPUT'"+
        "\nWhen -1 Then 'OUTPUT BY REF'"+
        "\nend type_param,"+
        "\ncase b.RDB\$FIELD_TYPE"+
        "\nwhen 7 then 'SMALLINT'"+
        "\nwhen 8 then 'INTEGER'"+
        "\nwhen 10 then 'FLOAT'"+
        "\nwhen 12 then 'DATE'"+
        "\nwhen 13 then 'TIME'"+
        "\nwhen 14 then 'CHAR'"+
        "\nwhen 16 then 'BIGINT'"+
        "\nwhen 27 then 'DOUBLE PRECISION'"+
        "\nwhen 35 then 'TIMESTAMP'"+
        "\nwhen 37 then 'VARCHAR'"+
        "\nWhen 40 Then 'CSTRING'"+
        "\nwhen 261 then 'BLOB'"+
        "\nend Type_Name,"+
        "\nb.RDB\$FIELD_SCALE as scale,"+
        "\nb.RDB\$FIELD_SUB_TYPE as subType,"+
        "\nb.RDB\$FIELD_LENGTH As Length,"+
        "\na.RDB\$DESCRIPTION as Description_function,"+
        "\nb.rdb\$description as description_param"+
        "\nFROM RDB\$FUNCTIONS a"+
        "\nLeft join RDB\$FUNCTION_ARGUMENTS b on a.RDB\$FUNCTION_NAME = b.RDB\$FUNCTION_NAME"+
        "\nwhere a.RDB\$FUNCTION_NAME = ?"

        executeReq(query, arrayOf(udfName))
    }

    /**
     * get input data
     * @return _inputData
     */
    fun getInput() : Array<Any> {
        return _inputData
    }

    /**
     * get output data
     * @return _outputData
     */
    fun getOutputData() : Array<MutableMap<String, Any>> {
        return _outputData
    }

    /**
     * Return the number of line in output data
     * @return _nbOutputLine
     */
    fun getNbOutputLine() : Int {
        return _nbOutputLine
    }

    /**
     * Return the number of column in output data
     * @return _nbOutputCol
     */
    fun getNbOutputCol() : Int {
        return _nbOutputCol
    }

    /**
     * Add data in _inputData
     * @param data - data we want add in any format
     */
    fun addInput(data : Any) {
        _inputData = _inputData.plus(data)
    }

    /**
     * Rem the first element of _inputData
     */
    fun remInput() {
        var tmp : Array<Any> = emptyArray()
        for ( i in 1 until _inputData.size)
        {
            tmp = tmp.plus(_inputData[i])
        }
        _inputData = tmp
    }

    /**
     * Connect database
     * @param dbName="DB.FDB" - Name of database file
     * @param user="SYSDBA" - username
     * @param pass="masterkey" - user's password
     * @param useUsersFolders=true - the database file is in user's folders or in root folders
     */
    fun connectJava(dbName : String = "DB.FDB", user : String = "SYSDBA", pass : String = "masterkey") {
        if (_useUsersFolders) // db is in /storage/emulated/0/Android/data/<packageName>/files folder and can be accessible by the user
            connect(File(_context.getExternalFilesDir(null), dbName).absolutePath, user, pass)
        else // db is in /data/data/<packageName>/files folder and can't be accesible by the user
            connect(File(_context.filesDir, dbName).absolutePath, user, pass)
    }

    /**
     * Disconnect the database
     */
    fun disconnectJava() {
        disconnect()
    }

    /**
     * Execute request with _inputData for input and return _outputData
     * @param req - SQL request
     */
    fun executeReq(req : String) : Array<MutableMap<String, Any>> {
        try {
            return executeReq(req, _inputData)
        }
        catch (e: Exception)
        {
            throw e
        }
    }

    /**
     * Execute request and return _outputData
     * @param req - SQL request
     * @param inData - Array of input data
     */
    fun executeReq(req : String, inData : Array<Any>) : Array<MutableMap<String, Any>> {
        try {
            for (i in inData.indices) {
                inData[i] = inData[i].toString()
            }

            val dataString = executeReqC(req, inData)

            _outputData = emptyArray()
            var map = mutableMapOf<String, Any>()

            if (dataString == null) //If null (can be true)
            {
                _outputData = _outputData.plus(map)
                _nbOutputLine = -1
                _nbOutputCol = -1
            }
            else if (dataString.size == 1) //only header info (=no data)
            {
                map = mutableMapOf()
                for (j in 0 until dataString[0].size) {
                    map[dataString[0][j][0]] = ""
                }
                _outputData = _outputData.plus(map)
                _nbOutputLine = 0
                _nbOutputCol = dataString[0].size
            }
            else
            {
                _nbOutputLine = dataString.size-1 //remove header line
                _nbOutputCol = dataString[0].size
                for (i in 1 until dataString.size) {
                    map = mutableMapOf()
                    for (j in 0 until dataString[i].size) {
                        val type = dataString[i][j][1].toInt()

                        map[dataString[0][j][0]] = when (type) {
                            _SQL_TYPE["MY_NULL"] as Int ->  {""}
                            _SQL_TYPE["SQL_TEXT"] as Int -> {dataString[i][j][0].replace("\r\n", "")}
                            _SQL_TYPE["SQL_VARYING"] as Int -> {dataString[i][j][0]}
                            _SQL_TYPE["SQL_SHORT"] as Int -> {dataString[i][j][0].toInt()}
                            _SQL_TYPE["SQL_LONG"] as Int -> {dataString[i][j][0].toLong()}
                            _SQL_TYPE["SQL_FLOAT"] as Int -> {dataString[i][j][0].toFloat()}
                            _SQL_TYPE["SQL_DOUBLE"] as Int -> {dataString[i][j][0].toDouble()}
                            _SQL_TYPE["SQL_TIMESTAMP"] as Int -> {dataString[i][j][0]}
                            _SQL_TYPE["SQL_BLOB"] as Int -> {dataString[i][j][0].replace("\r\n", "")}
                            _SQL_TYPE["SQL_ARRAY"] as Int-> {dataString[i][j][0].replace("\r\n", "")}
                            _SQL_TYPE["SQL_TYPE_TIME"] as Int -> {dataString[i][j][0]}
                            _SQL_TYPE["SQL_TYPE_DATE"] as Int -> {dataString[i][j][0]}
                            _SQL_TYPE["SQL_INT64"] as Int -> {dataString[i][j][0].toLong()}
                            _SQL_TYPE["SQL_TIMESTAMP_TZ"] as Int -> {dataString[i][j][0]} //timestamp with timezone
                            _SQL_TYPE["SQL_BOOLEAN"] as Int -> {dataString[i][j][0].toBoolean()}
                            else -> {dataString[i][j][0]} //OTHER TYPE
                        }
                    }
                    _outputData = _outputData.plus(map)
                }
            }

            return _outputData
        }
        catch (e: Exception)
        {
            throw e
        }
    }
}
