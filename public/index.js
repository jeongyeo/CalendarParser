// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    //init
    var filenames = new Array();
    updateLists();
    updateLogTable();
    updateCalEventListTable();

    function getStatusLine(){
        $.ajax({
            type: 'get',
            url: '/getStatusLine',
            success: function(data){
                console.log(data);
                sendToLog(data);
            },
            fail: function(e){
                console.log(e);
            },
        });
    }

    function resetTables(){
        $.ajax({
            type: 'get',
            url: '/resetSQLTables',
            success: function(data){
                console.log(data);
                sendToLog("Reset all tables");
            },
            fail: function(e){
                console.log(e);
            },
        });
    }

    function uploadFileToSQL(filename){
        $.ajax({
            type: 'get',
            url: '/uploadFileToSQL',
            data: {filename},
            success: function(data){
                console.log(data);
                sendToLog("Updated file " + filename + " to server");
            },
            fail: function(e){
                console.log(e);
            },
        });
    }

    function connectDB(uname, pwd, dbname){
        $.ajax({
            type: 'get',
            url: '/connectDB',
            data: {"id": uname,
                   "password": pwd,
                   "db": dbname},
            dataType: 'json',
            success: function(data){
                if(data.status == "Error"){
                    sendToLog("Invalid credentials");
                }
                else{
                    sendToLog("Connected to mysql database");
                    if(filenames.length > 0){
                        document.getElementById('storeAllFilesButton').disabled = false;
                        document.getElementById('resetAllTablesButton').disabled = false;
                        document.getElementById('getStatusLineButton').disabled = false;
                        document.getElementById('executeQueryButton').disabled = false;
                        for(var i = 0; i < filenames.length; i++){
                            var o = new Option(filenames[i], filenames[i]);
                            $(o).html(filenames[i]);
                            $("#queryFileOptions").append(o);
                        }
                    }
                }
            },
            fail: function(e){
                console.log(e);
            },
        });
    }
    
    function addEvent(){
        var filename = $('#calendarList1').find(":selected").text();
        var uid = document.getElementById('ceUID').value;
        var dtSTAMP = document.getElementById('ceDTSTAMP').value;
        var dtSTART = document.getElementById('ceDTSTART').value;
        if(filename == "" || uid == "" || dtSTAMP == "" || dtSTAMP == ""){
            sendToLog("Empty value recieved while adding event");
            return;
        }
        addEventAJAX(filename, uid, dtSTAMP, dtSTART);
    }

    function addEventAJAX(filename, uid, dtSTAMP, dtSTART){
        $.ajax({
            type: 'get',
            url: '/addEvent',
            data: {'filename': filename,
                   'uid': uid,
                   'dtSTAMP': dtSTAMP,
                   'dtSTART': dtSTART},
            dataType: 'json',
            success: function(data){
                if(data.error == "OK"){
                    updateCalEventListTable();
                    updateLogTable();
                    $('#ceUID').val("");
                    $('#ceDTSTAMP').val("");
                    $('#ceDTSTART').val("");
                    sendToLog("Successfully added event to " + filename);
                }
                else{
                    sendToLog("Error adding event due to " + data.error);
                }
            },
            fail: function(error){
                console.log(error);
            },
        });
    }

    function createCalendar(){
        var filename = document.getElementById('ccFileName').value;
        var version = document.getElementById('ccVersion').value;
        var pid = document.getElementById('ccPID').value;
        var uid = document.getElementById('ccUID').value;
        var dtSTAMP = document.getElementById('ccDTSTAMP').value;
        var dtSTART = document.getElementById('ccDTSTART').value;
        var tempFilename = filename.split(/(\\|\/)/g).pop();
        if(filename == "" || version == "" || pid == "" || uid == "" || dtSTAMP == "" || dtSTAMP == ""){
            sendToLog("Empty value recieved while adding calendar");
            return;
        }
        for(var i = 0; i < filenames.length; i++){
            if(filenames[i] == tempFilename){
                sendToLog("Failed to upload. File already exists on server");
                return;
            }
        }
        var extention = tempFilename.split('.').pop();
        if(extention != 'ics'){
            sendToLog("Failed to upload. File exention is not .ics");
            return;
        }
        createCalendarAJAX(filename, version, pid, uid, dtSTAMP, dtSTART);
    }

    function createCalendarAJAX(filename, version, pid, uid, dtSTAMP, dtSTART){
        $.ajax({
            type: 'get',
            url: '/createCalendar',
            data: {'filename': filename,
                   'version': version,
                   'pid': pid,
                   'uid': uid,
                   'dtSTAMP': dtSTAMP,
                   'dtSTART': dtSTART},
            dataType: 'json',
            success: function(data){
                if(data.error == "OK"){
                    updateFileNames();
                    updateLists();
                    updateCalEventListTable();
                    updateLogTable();
                    $('#ccFileName').val("");
                    $('#ccVersion').val("");
                    $('#ccPID').val("");
                    $('#ccUID').val("");
                    $('#ccDTSTAMP').val("");
                    $('#ccDTSTART').val("");
                    sendToLog("Created and uploaded file " + filename);
                }
                else{
                    sendToLog("Error creating file due to " + data.error);
                }
            },
            fail: function(error){

            },
        });
    }
    
    function displayEventProps(){
        var eventNo = $('#eventNumberInput').val();
        var filename = $('#calendarList1').find(":selected").text();
        getCalendarEventPropAJAX(filename, eventNo);
    }

    function getCalendarEventPropAJAX(filename, eventNo){
        $.ajax({
            type: 'get',
            url: '/getEventPropList',
            data: {'filename': filename,
                   'eventNo': eventNo},
            dataType: 'json',
            success: function(data){
                if(data.hasOwnProperty("error")){
                    if(data.error == "INV_EVENTNO"){
                        sendToLog("Invalid event number");
                    }
                    else{
                        sendToLog("Error parsing file. " + data.error);
                    }
                }
                else{
                    sendToLog("Properties of event " + eventNo + " of " + filename);
                    if(data.length == 0){
                        sendToLog("No event properties to show");
                    }
                    else{
                        for(var i = 0; i < data.length; i++){
                            sendToLog(data[i].prop);
                        }
                    }
                }
                console.log(data);
            },
            fail: function(error){
                console.log(error);
            },
        });
    }

    function displayAlarms(){
        var eventNo = $('#eventNumberInput').val();
        var filename = $('#calendarList1').find(":selected").text();
        getCalendarEventAlarmAJAX(filename, eventNo);
    }

    function getCalendarEventAlarmAJAX(filename, eventNo){
        $.ajax({
            type: 'get',
            url: '/getAlarmList',
            data: {'filename': filename,
                   'eventNo': eventNo},
            dataType: 'json',
            success: function(data){
                if(data.hasOwnProperty("error")){
                    if(data.error == "INV_EVENTNO"){
                        sendToLog("Invalid event number");
                    }
                    else{
                        sendToLog("Error parsing file. " + data.error);
                    }
                }
                else{
                    sendToLog("Alarms of event " + eventNo + " of " + filename);
                    if(data.length == 0){
                        sendToLog("No alarms to show");
                    }
                    else{
                        for(var i = 0; i < data.length; i++){
                            sendToLog("Alarm number " + (i + 1) + ": Action: " + data[i].action + " Trigger: " + data[i].trigger + " Number of Properties: " + data[i].numProps);
                        }
                    }
                }
                console.log(data);
            },
            fail: function(error){
                console.log(error);
            },
        })
    }

    function updateCalEventListTable(){
        var filename = $('#calendarList1').find(":selected").text();
        $("#calendarEventListTable").find("tr:gt(0)").remove();
        if(filenames.length == 0){
            $('#calendarEventListTable').append('<tr>' +
                                          '<td>No files</td>' + 
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '</tr>');
            return;
        }
        getCalendarEventAJAX(filename);
    }

    function getCalendarEventAJAX(filename){
        var i;
        $.ajax({
            type: 'get',
            url: '/getEventList',
            data: {'filename': filename},
            dataType: 'json',
            success: function(data){
                if(data.hasOwnProperty("error")){
                    $('#calendarEventListTable').append('<tr>' +
                                      '<td>' + filename + '</td>' + 
                                      '<td>' + data.error + '</td>' +
                                      '<td></td>' +
                                      '<td></td>' +
                                      '<td></td>' +
                                      '<td></td>' +
                                      '</tr>');
                    return;
                }
                for(i = 0; i < data.length; i++){
                    var date = data[i].startDT.date;
                    var time = data[i].startDT.time;
                    var utc = data[i].startDT.isUTC;
                    var finalDate = date.slice(0, 4) + '/' + date.slice(4, 6) + '/' + date.slice(6, 8);
                    var parsedTime = time.match(/.{2}/g);
                    var finalTime = parsedTime[0] + ':' + parsedTime[1] + ':' + parsedTime[2];
                    if(utc)
                        finalTime = finalTime + (' (UTC)');
                    $('#calendarEventListTable').append('<tr>' +
                                          '<td>' + (i + 1) + '</td>' + 
                                          '<td>' + finalDate + '</td>' +
                                          '<td>' + finalTime + '</td>' +
                                          '<td>' + data[i].summary + '</td>' +
                                          '<td>' + data[i].numProps + '</td>' +
                                          '<td>' + data[i].numAlarms + '</td>' +
                                          '</tr>');
                }                    
                console.log(data);
            },
            fail: function(error){
                console.log(error);
            },
        })
    }

    function updateLogTable(){
        updateFileNames();
        $("#fileLogTable").find("tr:gt(0)").remove();
        if(filenames.length == 0){
            $('#fileLogTable').append('<tr>' +
                                          '<td>No files</td>' + 
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '<td>No files</td>' +
                                          '</tr>');
        }
        else{
            var i;
            for(i = 0; i < filenames.length; i++){
                getCalendarAJAX(filenames[i]);
            }
        }
    }

    function getCalendarAJAX(filename){
        $.ajax({
            type: 'get',
            url: '/getCalendar',
            data: {'filename': filename},
            dataType: 'json',
            success: function(data){
                if(data.hasOwnProperty("error")){
                    $('#fileLogTable').append('<tr>' +
                                          '<td>' + filename + '</td>' + 
                                          '<td></td>' +
                                          '<td>Invalid file Error: ' + data.error + '</td>' +
                                          '<td></td>' +
                                          '<td></td>' +
                                          '</tr>');
                }
                else{
                    $('#fileLogTable').append('<tr>' +
                                          '<td>' + '<a href=\"uploads/' + filename + '\">' + filename + '</a></td>' + 
                                          '<td>' + data.version + '</td>' +
                                          '<td>' + data.prodID + '</td>' +
                                          '<td>' + data.numEvents + '</td>' +
                                          '<td>' + data.numProps + '</td>' +
                                          '</tr>');
                }
                console.log(data);
            },
            fail: function(error){
                console.log(error);
            },
        });
    }

    function updateLists(){
        var i;
        updateFileNames();
        $("#calendarList1").empty();
        if(filenames.length == 0){
            var o = new Option("No files", "No files");
            $(o).html("No Files");
            $("#calendarList1").append(o);
            var o = new Option("No files", "No files");
            $(o).html("No Files");
            $("#calendarList2").append(o);
            return;
        }
        for(i = 0; i < filenames.length; i++){
            var o = new Option(filenames[i], filenames[i]);
            $(o).html(filenames[i]);
            $("#calendarList1").append(o);
        }
        $("#calendarList2").empty();
        for(i = 0; i < filenames.length; i++){
            var o = new Option(filenames[i], filenames[i]);
            $(o).html(filenames[i]);
            $("#calendarList2").append(o);
        }
    }

    function updateFileNames(){
        $.ajax({
            type: 'get',
            url: '/getFileNames',
            async: false,
            success: function(data){
                filenames = data;
                console.log(data);
            },
            fail: function(error){
                console.log(error);
            }
        });
    }

    function sendToLog(newText){
        var oldText = document.getElementById("statusLog").innerHTML;
        document.getElementById("statusLog").innerHTML = oldText + '<br>' + newText;
        var objDiv = document.getElementById("statusLogContainer");
        objDiv.scrollTop = objDiv.scrollHeight; 
    }

    function uploadFile(){
        let formData = new FormData();
        var fileName = document.getElementById("fileSelectInput").value.split(/(\\|\/)/g).pop();
        for(var i = 0; i < filenames.length; i++){
            if(filenames[i] == fileName){
                sendToLog("Failed to upload. File already exists on server");
                return;
            }
        }
        var extention = fileName.split('.').pop();
        if(extention != 'ics'){
            sendToLog("Failed to upload. File exention is not .ics");
            return;
        }
        formData.append("uploadFile", document.getElementById("fileSelectInput").files[0]);
        $.ajax({
            type: 'post',           
            url: '/upload',
            data: formData,
            async: true,
            cache: false,
            processData: false,
            contentType: false,
            success: function(data){
                sendToLog("Successfully uploaded file " + fileName);
            },
            error: function(){
                sendToLog("Failed to upload file " + fileName);
            }
        });
    }

    //event list table event listener
    $('#calendarList1').change(function(e){
        e.preventDefault();
        updateCalEventListTable();
    });

    // clear status event listener
    $('#clearLogButton').click(function(e){
        e.preventDefault();
        $('#statusLog').text('');
    });

    //upload file event listener
    $('#fileUploadButton').click(function(e){
        e.preventDefault();
        uploadFile();
        updateFileNames();
        updateLists();
        if(filenames.length == 1){
            sendToLog("Displaying list of events for " + $('#calendarList1').find(":selected").text());
        }
        updateCalEventListTable();
        updateLogTable();
    });

    //display alarm event listener
    $('#displayAlarmsButton').click(function(e){
        e.preventDefault();
        sendToLog("");
        displayAlarms();
    });

    //display optional properties event listener
    $('#displayOpPropButton').click(function(e){
        e.preventDefault();
        sendToLog("");
        displayEventProps();
    });

    //create calendar event listener
    $('#createCalendarButton').click(function(e){
        e.preventDefault();
        createCalendar();
    });

    //add event event listener
    $('#addEventButton').click(function(e){
        e.preventDefault();
        addEvent();
    });

    //db login event listener
    $('#dbsubmit').click(function(e){
        e.preventDefault();
        var uname = document.getElementById('dbuname').value;
        var pwd = document.getElementById('dbpsw').value;
        var dbname = document.getElementById('dbname').value;
        if(uname != "" && pwd != "" && dbname != ""){
            connectDB(uname, pwd, dbname);
        }
        else{
            sendToLog("ID or password empty");
        }
        $('#dbuname').val("");
        $('#dbpsw').val("");
        $('#dbname').val("");
    });

    $('#storeAllFilesButton').click(function(e){
        e.preventDefault();
        let i;
        for(i = 0; i < filenames.length; i++){
            uploadFileToSQL(filenames[i]);
        }
    });

    $('#resetAllTablesButton').click(function(e){
        e.preventDefault();
        resetTables();
    });

    $('#getStatusLineButton').click(function(e){
        e.preventDefault();
        getStatusLine();
    });

    $('#queryOptions').change(function(e){
        e.preventDefault();
        var option = $('#queryOptions').find(":selected").val();
        if(option == 2 || option == 6){
            document.getElementById('getQueryCalID').disabled = false;
        }
        else{
            document.getElementById('getQueryCalID').disabled = true;
        }
        if(option == 4){
            document.getElementById('getQueryEventID').disabled = false;
        }
        else{
            document.getElementById('getQueryEventID').disabled = true;
        }
    });

    $('#executeQueryButton').click(function(e){
        e.preventDefault();
        executeQueryOption();
    });

    function executeQueryOption(){
        var option = $('#queryOptions').find(":selected").val();
        var cal_id = document.getElementById('getQueryCalID').value;
        var event_id = document.getElementById('getQueryEventID').value;
        $.ajax({
            type: 'get',
            url: '/executeQueryOption',
            data: {option, cal_id, event_id},
            success: function(data){
                if(option == 1 || option == 2 || option == 3 || option == 5){
                    $("#dbTable").empty();
                    $('#dbTable').append('<tr>' +
                                          '<th>event_id</th>' + 
                                          '<th>summary</th>' +
                                          '<th>start_time</th>' +
                                          '<th>location</th>' +
                                          '<th>organizer</th>' +
                                          '<th>cal_file</th>' +
                                          '</tr>');
                    for(var i = 0; i < data.length; i++){
                       $('#dbTable').append('<tr>' +
                                            '<td>' + data[i].event_id + '</td>' + 
                                            '<td>' + data[i].summary + '</td>' +
                                            '<td>' + data[i].start_time + '</td>' +
                                            '<td>' + data[i].location + '</td>' +
                                            '<td>' + data[i].organizer + '</td>' +
                                            '<td>' + data[i].cal_file + '</td>' +
                                            '</tr>');
                    }
                }
                else if(option == 4){
                    $("#dbTable").empty();
                    $('#dbTable').append('<tr>' +
                                          '<th>alarm_id</th>' + 
                                          '<th>action</th>' +
                                          '<th>trigger</th>' +
                                          '<th>event</th>' +
                                          '</tr>');
                    for(var i = 0; i < data.length; i++){
                       $('#dbTable').append('<tr>' +
                                            '<td>' + data[i].alarm_id + '</td>' + 
                                            '<td>' + data[i].action + '</td>' +
                                            '<td>' + data[i].trigger + '</td>' +
                                            '<td>' + data[i].event + '</td>' +
                                            '</tr>');
                    }
                    
                }
                console.log(data);
            },
            fail: function(e){
                console.log(e);
            },
        });
    }

});