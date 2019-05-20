'use strict'
// mySQL
const mysql = require('mysql');
let connection;

// C library API
const ffi = require('ffi');

let sharedLib = ffi.Library('./library.so', {
  'json_from_createCalendar': ['string', ['string']],
  'json_from_eventList': ['string', ['string']],
  'json_from_alarmList': ['string', ['string', 'int']],
  'json_from_eventPropList': ['string', ['string', 'int']],
  'calendar_from_json': ['string', ['string', 'string', 'string', 'string', 'string']],
  'addEventToFile': ['string', ['string', 'string', 'string', 'string']],
});

let file_table = "CREATE TABLE IF NOT EXISTS `FILE` (" +
  "`cal_id` int(11) NOT NULL AUTO_INCREMENT," +
  "`file_Name` varchar(60) NOT NULL," +
  "`version` int(11) NOT NULL," +
  "`prod_id` varchar(256) NOT NULL," +
  "PRIMARY KEY (`cal_id`)" +
  ") ENGINE=InnoDB AUTO_INCREMENT=53 DEFAULT CHARSET=latin1";

let event_table = "CREATE TABLE IF NOT EXISTS `EVENT` (" +
  "`event_id` int(11) NOT NULL AUTO_INCREMENT," +
  "`summary` varchar(1024) DEFAULT NULL," +
  "`start_time` datetime NOT NULL," +
  "`location` varchar(60) DEFAULT NULL," +
  "`organizer` varchar(256) DEFAULT NULL," +
  "`cal_file` int(11) NOT NULL," +
  "PRIMARY KEY (`event_id`)," +
  "KEY `cal_file` (`cal_file`)," +
  "CONSTRAINT `EVENT_ibfk_1` FOREIGN KEY (`cal_file`) REFERENCES `FILE` (`cal_id`) ON DELETE CASCADE" +
  ") ENGINE=InnoDB AUTO_INCREMENT=56 DEFAULT CHARSET=latin1;";

let alarm_table = "CREATE TABLE IF NOT EXISTS `ALARM` (" +
  "`alarm_id` int(11) NOT NULL AUTO_INCREMENT," +
  "`action` varchar(256) NOT NULL," +
  "`trigger` varchar(256) NOT NULL," +
  "`event` int(11) NOT NULL," +
  "PRIMARY KEY (`alarm_id`)," +
  "KEY `event` (`event`)," +
  "CONSTRAINT `ALARM_ibfk_1` FOREIGN KEY (`event`) REFERENCES `EVENT` (`event_id`) ON DELETE CASCADE" +
  ") ENGINE=InnoDB AUTO_INCREMENT=9 DEFAULT CHARSET=latin1";

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');

app.use(fileUpload());

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }
 
  let uploadFile = req.files.uploadFile;
 
  // Use the mv() method to place the file somewhere on your server
  uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    if(err) {
      return res.status(500).send(err);
    }

    res.redirect('/');
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    console.log(err);
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      res.send('');
    }
  });
});

//******************** Your code goes here ******************** 
app.get('/createCalendar', function(req, res){
  var calObj = '{'
            +'"version":' + req.query.version + ','
            +'"prodID":"' + req.query.pid
            +'"}';
  var eveObj = '{'
            +'"uid":"' + req.query.uid + '"}';
  res.send(sharedLib.calendar_from_json(req.query.filename, req.query.dtSTAMP, req.query.dtSTART, calObj, eveObj));
});

app.get('/addEvent', function(req, res){
  var eveObj = '{'
            +'"uid":"' + req.query.uid + '"}';
  res.send(sharedLib.addEventToFile(req.query.filename, req.query.dtSTAMP, req.query.dtSTART, eveObj));
});

app.get('/getFileNames', function(req, res){
  fs.readdir('./uploads/', function(req, file){
    res.send(file);
  });
});

app.get('/getCalendar', function(req, res){
  res.send(sharedLib.json_from_createCalendar("./uploads/" + req.query.filename));
});

app.get('/getEventList', function(req, res){
  res.send(sharedLib.json_from_eventList("./uploads/" + req.query.filename));
});

app.get('/getAlarmList', function(req, res){
  res.send(sharedLib.json_from_alarmList("./uploads/" + req.query.filename, req.query.eventNo));
});

app.get('/getEventPropList', function(req, res){
  res.send(sharedLib.json_from_eventPropList("./uploads/" + req.query.filename, req.query.eventNo));
});

app.get('/connectDB', function(req, res){
  connection = mysql.createConnection({
    host     : 'dursley.socs.uoguelph.ca',
    user     : req.query.id,
    password : req.query.password,
    database : req.query.database,
  });

  connection.connect(function(err){
    if(err){
      res.send("{\"status\":\"Error\"}");
    }
    else{
      connection.query("use jeongyeo", function (err, rows, fields) {
        if(err){
          res.send("{\"status\":\"Error\"}");
        }
      res.send("{\"status\":\"Connected\"}");
      });
    }
  });
});

app.get('/uploadFileToSQL', function(req, res){
  createTables();
  insertFile(req.query.filename);
  res.send("Successfully uploaded file " + req.query.filename).status(200);
});

app.get('/resetSQLTables', function(req, res){
  resetTables();
  res.send("Successfully reset tables");
});

app.get('/getStatusLine', function(req, res){
  var query = "SELECT * FROM FILE";
  connection.query(query, function(err, rows, fields){
    if(err) throw err;
    else{
      var fileLength = rows.length;
      query = "SELECT * FROM EVENT";
      connection.query(query, function(err, rows){
        if(err) throw err;
        else{
          var eventLength = rows.length;
          query = "SELECT * FROM ALARM";
          connection.query(query, function(err, rows){
            if(err) throw err;
            else{
              var alarmLength = rows.length;
              res.send("Database has " + fileLength + " files, " + eventLength + " events, and " + alarmLength + " alarms.");
            }
          });
        }
      });
      
    }
  });
});

function resetTables(){
  var query = "DELETE FROM FILE;";
  runQuery(query);
  query = "ALTER TABLE FILE AUTO_INCREMENT = 1";
  runQuery(query);
  query = "DELETE FROM EVENT";
  runQuery(query);
  query = "ALTER TABLE EVENT AUTO_INCREMENT = 1";
  runQuery(query);
  query = "DELETE FROM ALARM";
  runQuery(query);
  query = "ALTER TABLE ALARM AUTO_INCREMENT = 1";
  runQuery(query);
}

function createTables(){
  runQuery(file_table);
  runQuery(event_table);
  runQuery(alarm_table);
}

function insertFile(filename){
  let fileData = JSON.parse(sharedLib.json_from_createCalendar("./uploads/" + filename));
  let query1 = "INSERT IGNORE INTO FILE (file_name, version, prod_id) VALUES ('" + filename + "', '" + fileData.version + "', '" + fileData.prodID + "')";
  let query2 = "SELECT file_name FROM FILE WHERE file_name = '" + filename + "'";
  connection.query(query2, function(err, rows, fields){
    if(err) throw err;
    if(rows.length > 0){
      console.log("File already uploaded to db");
    }
    else{
      //insert to FILE
      runQuery(query1);
      //get cal_id
      var query = "SELECT cal_id FROM FILE WHERE file_name = '" + filename + "'";
      connection.query(query, function(err, rows, fields){
        if(err) throw err;
        else{
          for(let row of rows){
            var cal_id = row.cal_id;
            //insert EVENT
            var eventData = JSON.parse(sharedLib.json_from_eventList("./uploads/" + filename));
            for(let i = 0; i < eventData.length; i++){
              var eventPropData = sharedLib.json_from_eventPropList("./uploads/" + filename, i + 1);
              if(eventData[i].summary == null)
                var summary = 'NULL';
              else
                summary = eventData[i].summary;
              if(eventPropData.location == null)
                var location = 'NULL';
              else
                location = eventPropData.location;
              if(eventPropData.organizer == null)
                var organizer = 'NULL';
              else
                organizer = eventPropData.organizer;
              var year = eventData[i].startDT.date.slice(0, 4);
              var month = eventData[i].startDT.date.slice(4, 6);
              var date = eventData[i].startDT.date.slice(6, 8);
              var hour = eventData[i].startDT.time.slice(0, 2);
              var minutes = eventData[i].startDT.time.slice(2, 4);
              var seconds = eventData[i].startDT.time.slice(4, 6);
              var date = year + '-' + month + '-' + date  + ' ' + hour  + ':' + minutes + ':' + seconds;
              var query = "INSERT INTO EVENT (summary, start_time, location, organizer, cal_file) VALUES ('" + 
              summary + "', '" + date + "', '" + location  + "', '" + organizer  + "', '" + cal_id + "')";
              runQuery(query);
              //get event_id
              query = "SELECT event_id FROM EVENT WHERE cal_file = " + cal_id;
              connection.query(query, function(err, rows, fields){
                for(let row of rows){
                  //insert ALARM
                  var alarmData = JSON.parse(sharedLib.json_from_alarmList("./uploads/" + filename, i + 1));
                  for(var j = 0; j < alarmData.length; j++){
                    var query = "INSERT INTO ALARM (action, `trigger`, event) VALUES ('" +
                    alarmData[j].action + "', '" + alarmData[j].trigger  + "', " + row.event_id + ")";
                    runQuery(query);
                  }
                }
              });
            }
          }
        }
      });
    }
  });
}

app.get('/executeQueryOption', function(req, res){
  var option = req.query.option;
  var cal_id = req.query.cal_id;
  var event_id = req.query.event_id;
  if(option == 1){
    var query = "SELECT * FROM EVENT ORDER BY start_time";
  }
  else if(option == 2){
    var query = "SELECT * FROM EVENT WHERE cal_file = " + cal_id;  
  }
  else if(option == 3){
    var query = "SELECT * FROM EVENT WHERE start_time IN (SELECT start_time FROM EVENT GROUP BY start_time HAVING COUNT(*) >= 2)";
  }
  else if(option == 4){
    var query = "SELECT * FROM ALARM WHERE event = " + event_id;
  }
  else if(option == 5){
    var query = "SELECT * FROM EVENT WHERE location = 'NULL'";
  }
  else if(option == 6){
    var query = "DELETE FROM FILE WHERE cal_id = " + cal_id;
  }
  connection.query(query, function(err, rows, fields){
    res.send(rows);
  });
});

function runQuery(query){
  connection.query(query, function (err, rows, fields) {
    if(err) throw err;
  });
}

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);