package com.example.menbird

import android.content.ContextWrapper
import android.os.Bundle
import android.os.StrictMode
import androidx.activity.compose.setContent
import androidx.appcompat.app.AppCompatActivity
import androidx.compose.foundation.border
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Divider
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.TextFieldValue
import androidx.compose.ui.unit.dp

class DebugInterface : AppCompatActivity() {
    private lateinit var firebird : Firebird
    private var displayOut : Boolean = false
    private var showType : Int = 0 //0= Any, 1= show table, 2= show procedure, 3= Show UDF
    private var fbVersion : Array<String> = emptyArray()
    private var req : String = ""
    private var input : String = ""
    private var textError : String = "No error"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val policy = StrictMode.ThreadPolicy.Builder().permitAll().build()
        StrictMode.setThreadPolicy(policy)

        firebird = Firebird(baseContext as ContextWrapper, true)
        firebird.connectJava("DB.FDB")
        fbVersion = firebird.getVersion()

        display()
    }

    override fun onDestroy() {
        firebird.disconnectJava()
        super.onDestroy()
    }

    private fun display()
    {
        if (!displayOut)
        {
            setContent {
                Column(
                    modifier = Modifier.fillMaxWidth(),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    Text(text = fbVersion[0], color = Color.Red)
                    Text(text = fbVersion[1], color = Color.Red)
                    ReqField()
                    RequestButton()
                    InputField()
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        AddInputButton()
                        RemInputButton()
                    }
                    Row {
                        TableNameButton()
                        ProcedureNameButton()
                        UDFNameButton()
                    }
                    Text(text = textError, color = Color.Red)
                    DisplayInTable()
                }
            }
        }
        else
        {
            setContent {
                Column {
                    Text(text = "$req : done with success", color = Color.Gray)
                    Button(onClick = { displayOut = false;showType = 0;display()}) {
                        Text(text = "Retour")
                    }
                    DisplayOutTable()
                }
            }
        }
    }

    @Composable
    private fun TableNameButton() {
        Button(onClick =
        {
            try {
                firebird.getTableName()
                textError = "No error"
                req = "Show table"
                displayOut = true
                showType = 1
            } catch (e: Exception) {
                textError = "Error: ${e.message}"
            }
            display()
        }) {
            Text(text = "table")
        }
    }

    @Composable
    private fun ProcedureNameButton()
    {
        Button(onClick =
        {
            try {
                firebird.getProcedureName()
                textError = "No error"
                req = "Show procedure"
                displayOut = true
                showType = 2
            } catch (e: Exception) {
                textError = "Error: ${e.message}"
            }
            display()
        }) {
            Text(text = "procedure")
        }
    }

    @Composable
    private fun UDFNameButton()
    {
        Button(onClick =
        {
            try {
                firebird.getUDFName()
                textError = "No error"
                req = "Show UDF"
                displayOut = true
                showType = 3
            } catch (e: Exception) {
                textError = "Error: ${e.message}"
            }
            display()
        }) {
            Text(text = "UDF")
        }
    }

    @OptIn(ExperimentalMaterial3Api::class)
    @Composable
    private fun ReqField() {
        var text by remember { mutableStateOf(TextFieldValue(req)) }
        OutlinedTextField(
            value = text,
            onValueChange = {
                text = it
                req = text.text
            },
            label = { Text(text = "SQL request") },
            placeholder = { Text(text = "enter SQL") },
        )
    }

    @Composable
    private fun RequestButton()
    {
        Button(onClick =
        {
            try
            {
                firebird.executeReq(req)
                textError = "No error"
                displayOut = true
            }
            catch (e: Exception)
            {
                textError  = "Error: ${e.message}"
            }
            display()
        }) {
            Text(text = "Executez la requete")
        }
    }

    @OptIn(ExperimentalMaterial3Api::class)
    @Composable
    private fun InputField() {
        var text by remember { mutableStateOf(TextFieldValue("")) }
        OutlinedTextField(
            value = text,
            onValueChange = {
                text = it
                input = text.text
            },
            label = { Text(text = "Input data") },
            placeholder = { Text(text = "enter input data") },
        )
    }

    @Composable
    private fun AddInputButton()
    {
        Button(onClick = {
            firebird.addInput(input)
            display()
        }) {
            Text(text = "Add input")
        }
    }

    @Composable
    private fun RemInputButton()
    {
        Button(onClick = {
            firebird.remInput()
            display()
        }) {
            Text(text = "Rem input")
        }
    }

    @Composable
    private fun DisplayInTable() {
        val scrollStateVertical = rememberScrollState()
        val inData = firebird.getInput()
        Column(modifier = Modifier
            .fillMaxWidth()
            .border(1.dp, Color.Gray)
            .verticalScroll(scrollStateVertical),
            verticalArrangement = Arrangement.Center,
            horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                text = "Input Data",
                modifier = Modifier
                    .padding(8.dp),
                color = Color.Yellow,
                fontWeight = FontWeight.Bold
            )
            repeat(inData.size) {
                Divider(color = Color.Gray,
                    modifier = Modifier
                        .fillMaxWidth()
                        .width(1.dp))
                Text(
                    text = inData[it].toString(),
                    modifier = Modifier
                        .padding(8.dp),
                    color = Color.Red,
                    fontWeight = FontWeight.Normal
                )
            }
        }
    }

    @Composable
    private fun DisplayOutTable() {
        val scrollStateHorizontal = rememberScrollState()
        val scrollStateVertical = rememberScrollState()
        var first = 0

        Row(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(state = scrollStateVertical)
                .horizontalScroll(state = scrollStateHorizontal),
        ) {
            for (cols in firebird.getOutputData()[0].keys) {
                Column(modifier = Modifier
                    .width(IntrinsicSize.Max)
                    .border(1.dp, Color.Gray),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        text = cols,
                        modifier = Modifier
                            .height(50.dp)
                            .padding(8.dp),
                        color = Color.Yellow,
                        fontWeight = FontWeight.Bold
                    )
                    repeat(firebird.getNbOutputLine()) {
                        Divider(color = Color.Gray,
                            modifier = Modifier
                                .fillMaxWidth()
                                .width(1.dp))

                        val name : String = firebird.getOutputData()[it][cols].toString()

                        when (showType)
                        {
                            0 -> {
                                    Text(
                                        text = name,
                                        modifier = Modifier
                                            .height(50.dp)
                                            .padding(8.dp),
                                        color = Color.Red,
                                        fontWeight = FontWeight.Normal
                                    )
                                }
                            1 -> {
                                    Row(modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.SpaceEvenly)
                                    {
                                        Button(onClick =
                                        {
                                            try {
                                                firebird.getInfoOnTable(name)
                                                textError = "No error"
                                                req = "Show info of $name"
                                                displayOut = true
                                            } catch (e: Exception) {
                                                textError = "Error: ${e.message}"
                                                displayOut = false
                                            }
                                            showType = 0
                                            display()
                                        }) {
                                            Text(
                                                text = "Info of $name",
                                                modifier = Modifier
                                                    .height(50.dp)
                                                    .padding(8.dp),
                                                color = Color.Red,
                                                fontWeight = FontWeight.Normal
                                            )
                                        }
                                        Button(onClick =
                                        {
                                            try {
                                                firebird.executeReq("Select first 100 * from $name")
                                                textError = "No error"
                                                req = "Select first 100 * from $name"
                                                displayOut = true
                                            } catch (e: Exception) {
                                                textError = "Error: ${e.message}"
                                                displayOut = false
                                            }
                                            showType = 0
                                            display()
                                        }) {
                                            Text(
                                                text = "Data of $name",
                                                modifier = Modifier
                                                    .height(50.dp)
                                                    .padding(8.dp),
                                                color = Color.Red,
                                                fontWeight = FontWeight.Normal
                                            )
                                        }
                                    }
                                }
                            2 -> {
                                    Button(onClick =
                                    {
                                        try {
                                            firebird.getInfoOnProcedure(name)
                                            textError = "No error"
                                            req = "Show info of $name"
                                            displayOut = true
                                        } catch (e: Exception) {
                                            textError = "Error: ${e.message}"
                                            displayOut = false
                                        }
                                        showType = 0
                                        display()
                                    }) {
                                        Text(
                                            text = name,
                                            modifier = Modifier
                                                .height(50.dp)
                                                .padding(8.dp),
                                            color = Color.Red,
                                            fontWeight = FontWeight.Normal
                                        )
                                    }
                                }
                            3 -> {
                                    Button(onClick =
                                    {
                                        try {
                                            firebird.getInfoOnUDF(name)
                                            textError = "No error"
                                            req = "Show info of $name"
                                            displayOut = true
                                        } catch (e: Exception) {
                                            textError = "Error: ${e.message}"
                                            displayOut = false
                                        }
                                        showType = 0
                                        display()
                                    }) {
                                        Text(
                                            text = name,
                                            modifier = Modifier
                                                .height(50.dp)
                                                .padding(8.dp),
                                            color = Color.Red,
                                            fontWeight = FontWeight.Normal
                                        )
                                    }
                                }
                        }
                    }
                }
                first++
            }
        }
    }
}