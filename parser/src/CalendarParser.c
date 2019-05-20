/* 
 * Assignment: CIS2750 assign2
 * Author: Jeongyeon Park
 * ID: 1006554
 * Created on January 16, 2019, 5:35 PM
 */

#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "LinkedListAPI.h"
#include "CalendarParser.h"
#include "helper.h"

char *strdup(const char *s){
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p != NULL) {
        memcpy(p, s, size);
    }
    return p;
}

void freeArray(char **file, int length){
    int i;
    for(i = 0; i < length; i++){
        free(file[i]);
    }
    free(file);
}

Property *createProperty(char* string){
    Property *tempProp;
    char *tempName, *tempDiscr;
    
    if(strcspn(string, ":") > strcspn(string, ";"))
        tempName = strtok(string, ";");
    else
        tempName = strtok(string, ":");

    if(tempName == NULL)
        return NULL;
    tempDiscr = strtok(NULL, "\0");
    if(tempDiscr == NULL)
        return NULL;
    tempProp = malloc(sizeof(Property) + sizeof(char *) * strlen(tempDiscr));
    strcpy(tempProp->propName, tempName);
    strcpy(tempProp->propDescr, tempDiscr);
    return tempProp;
}

ICalErrorCode createCalendar(char *fileName, Calendar **obj){
    FILE *fp;
    Property *tempProp;
    Event *tempEvent;
    Alarm *tempAlarm;
    int arraySize = 0, lines = 0, i, j, calendarStartCounter = 0, calendarEndCounter = 0, eventStartCounter = 0, eventEndCounter = 0, alarmStartCounter = 0, alarmEndCounter = 0;
    char **file, *tempString, buffer[10000];
    bool isEvent = false, isAlarm = false, versionFound = false, prodidFound = false;

    if(fileName == NULL || !strcmp(fileName, ""))
        return INV_FILE;
    else
        fp = fopen(fileName, "r");
    
    if(!fp)
        return INV_FILE;
    else if(strcmp(strrchr(fileName, '.'), ".ics")){
        fclose(fp);
        return INV_FILE;
    }

    //parse line with line folding and put it in array of strings
    while(fgets(buffer, 10000, fp)){
        if(isspace((unsigned char) * buffer) || buffer[0] == ';')
            continue;
        arraySize++;
    }
    rewind(fp); //resets fp
    file = malloc((arraySize + 1) * sizeof(char *));
    while(fgets(buffer, 10000, fp)){
        if(strstr(buffer, "\r\n") != NULL)
            buffer[strcspn(buffer, "\r\n")] = 0; //parse out /r/n
        else{
            freeArray(file, lines);
            return INV_FILE;
        }
        if(buffer[0] == ';'){ //don't read if comment
            continue;
        }
        //append to string if folded line
        else if(buffer[0] == ' ' || buffer[0] == '\t'){
            if(buffer[0] == ' ' || buffer[0] == '\t'){
                memmove(buffer, buffer + 1, strlen(buffer)); //remove whitespace
            }
            //while(isspace((unsigned char) * buffer)) 
            file[lines - 1] = realloc(file[lines - 1], strlen(file[lines - 1]) + strlen(buffer) + 1);
            strcat(file[lines - 1], buffer);
        }
        //put into array
        else{
            file[lines++] = strdup(buffer);
        }
    }
    fclose(fp);
    
    //check opening/closing tags
    if(strcmp(file[0], "BEGIN:VCALENDAR") || strcmp(file[lines - 1], "END:VCALENDAR")){
        freeArray(file, lines);
        return INV_CAL;
    }
    
    // initiate Calendar object
    *obj = malloc(sizeof(Calendar));
    (*obj)->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
    (*obj)->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    
    //parse and put data into objects
    for(i = 0; i < lines; i++){
        if(!strcmp(file[i], "BEGIN:VCALENDAR")){
            calendarStartCounter++;
            continue;
        }
        if(!strcmp(file[i], "END:VCALENDAR")){
            calendarEndCounter++;
            continue;
        }
        if(isEvent){
            if(isAlarm){
                if(!strcmp(file[i], "END:VALARM")){
                    alarmEndCounter++;
                    isAlarm = false;
                    if(!strcmp(tempAlarm->action, "") || tempAlarm->trigger == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_ALARM;
                    }
                }
                else if(strstr(file[i], "ACTION") != NULL){
                    strtok(file[i], ":");
                    tempString = strtok(NULL, "\0");
                    if(tempString == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_ALARM;
                    }
                    strcpy(tempAlarm->action, tempString);
                }
                else if(strstr(file[i], "TRIGGER") != NULL){
                    strtok(file[i], ";");
                    tempString = strtok(NULL, "\0");
                    if(tempString == NULL){ //if delimitor is not ; it is :
                        strtok(file[i], ":");
                        tempString = strtok(NULL, "\0");
                    }
                    if(tempString == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_ALARM;
                    }
                    if((tempAlarm->trigger = malloc(sizeof(char *) + strlen(tempString))) == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return OTHER_ERROR;
                    }
                    strcpy(tempAlarm->trigger, tempString);
                }
                else{
                    if((tempProp = createProperty(file[i])) == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_ALARM;
                    }
                    else
                        insertBack(tempAlarm->properties, tempProp);
                }
            }
            else{
                if(!strcmp(file[i], "END:VEVENT")){
                    eventEndCounter++;
                    if(!strcmp(tempEvent->UID, "") || !strcmp(tempEvent->creationDateTime.date, "") || !strcmp(tempEvent->startDateTime.date, "")){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_EVENT;
                    }
                    isEvent = false;
                }
                else if(!strcmp(file[i], "BEGIN:VALARM")){
                    alarmStartCounter++;
                    isAlarm = true;
                    if((tempAlarm = malloc(sizeof(Alarm))) == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return OTHER_ERROR;
                    }
                    tempAlarm->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
                    insertBack(tempEvent->alarms, tempAlarm);
                }
                else if(strstr(file[i], "UID") != NULL){
                    strtok(file[i], ":");
                    tempString = strtok(NULL, "\0");
                    if(tempString == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_EVENT;
                    }
                    strcpy(tempEvent->UID, tempString); 
                }
                else if(strstr(file[i], "DTSTART") != NULL){
                    if(file[i][strlen(file[i]) - 1] == 'Z'){
                        tempEvent->startDateTime.UTC = true;
                    }
                    else
                        tempEvent->startDateTime.UTC = false;
                    strtok(file[i], ":");
                    tempString = strtok(strtok(NULL, "\0"), "T");
                    if(tempString == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_EVENT;
                    }
                    if(strlen(tempString) != 8){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_DT;
                    }
                    strcpy(tempEvent->startDateTime.date, tempString);
                    tempString = strtok(strtok(NULL, "\0"), "Z");
                    if(tempString == NULL || strlen(tempString) != 6){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_DT;
                    }
                    strcpy(tempEvent->startDateTime.time, tempString);
                }
                else if(strstr(file[i], "DTSTAMP") != NULL){
                    if(file[i][strlen(file[i]) - 1] == 'Z'){
                        tempEvent->creationDateTime.UTC = true;
                    }
                    else
                        tempEvent->creationDateTime.UTC = false;
                    strtok(file[i], ":");
                    tempString = strtok(strtok(NULL, "\0"), "T");
                    if(tempString == NULL || strlen(tempString) != 8){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_DT;
                    }
                    strcpy(tempEvent->creationDateTime.date, tempString);
                    tempString = strtok(strtok(NULL, "\0"), "Z");
                    if(tempString == NULL || strlen(tempString) != 6){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_DT;
                    }
                    strcpy(tempEvent->creationDateTime.time, tempString);
                }
                else{
                    if((tempProp = createProperty(file[i])) == NULL){
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_EVENT;
                    }
                    else
                        insertBack(tempEvent->properties, tempProp);
                }
            }
        }
        else{
            if(strstr(file[i], "BEGIN:VEVENT") != NULL){ //begin event
                eventStartCounter++;
                isEvent = true;
                if((tempEvent = malloc(sizeof(Event))) == NULL){
                    freeArray(file, lines);
                    deleteCalendar(*obj);
                    return OTHER_ERROR;
                }
                tempEvent->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
                tempEvent->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
                insertBack((*obj)->events, tempEvent);
            }
            else if(strstr(file[i], "VERSION") != NULL){
                if(versionFound){
                    freeArray(file, lines);
                    deleteCalendar(*obj);
                    return DUP_VER;
                }
                versionFound = true;
                strtok(file[i], ":");
                tempString = strtok(NULL, "\0");
                if(tempString == NULL){ //if property value is missing
                    freeArray(file, lines);
                    deleteCalendar(*obj);
                    return INV_VER;
                }
                for(j = 0; j < strlen(tempString); j++){
                    // check if it is digits
                    if(!isdigit(tempString[j]) && tempString[j] != '.'){ 
                        freeArray(file, lines);
                        deleteCalendar(*obj);
                        return INV_VER;
                    }
                }
                (*obj)->version = atof(tempString);
            }
            else if(strstr(file[i], "PRODID") != NULL){
                if(prodidFound){
                    freeArray(file, lines);
                    deleteCalendar(*obj);
                    return DUP_PRODID;
                }
                prodidFound = true;
                strtok(file[i], ":");
                tempString = strtok(NULL, "\0");
                if(tempString == NULL){
                    freeArray(file, lines);
                    deleteCalendar(*obj);
                    return INV_PRODID;
                }
                strcpy((*obj)->prodID, tempString);
            }
            else{
                if((tempProp = createProperty(file[i])) == NULL){
                    freeArray(file, lines);
                    deleteCalendar(*obj);
                    return INV_CAL;
                }
                else
                    insertBack((*obj)->properties, tempProp);
            }
        }
    }
    // if missing END:VALARM tag
    if(alarmStartCounter != alarmEndCounter){
        freeArray(file, lines);
        deleteCalendar(*obj);
        return INV_ALARM;
    }
    // if missing END:VEVENT tag
    if(eventStartCounter != eventEndCounter){
        freeArray(file, lines);
        deleteCalendar(*obj);
        return INV_EVENT;
    }
    //if calendar properties are missing
    if(prodidFound == false || versionFound == false || getLength((*obj)->events) < 1 || calendarStartCounter != calendarEndCounter){
        freeArray(file, lines);
        deleteCalendar(*obj);
        return INV_CAL;
    }
    freeArray(file, lines);
    return OK;
}

void deleteCalendar(Calendar* obj){
    freeList(obj->events);
    freeList(obj->properties);
    free(obj);
}

// return value must be freed by caller
char* printCalendar(const Calendar* obj){
    ListIterator eventIterator, propertyIterator;
    char *toReturn, *tempString;
    int i;

    if(obj == NULL)
        return NULL;

    // iterators
    eventIterator = createIterator(obj->events);
    propertyIterator = createIterator(obj->properties);
    //calendar info
    toReturn = malloc(snprintf(NULL, 0, "BEGIN:VCALENDAR\nVERSION:%.1f\nPRODID:%s\n", obj->version, obj->prodID) + 1); 
    sprintf(toReturn, "BEGIN:VCALENDAR\nVERSION:%.1f\nPRODID:%s\n", obj->version, obj->prodID);
    //calendar properties
    for(i = 0; i < getLength(obj->properties); i++){
        tempString = printProperty(nextElement(&propertyIterator));
        toReturn = realloc(toReturn, snprintf(NULL, 0, "%s%s\n", toReturn, tempString) + 1);
        sprintf(toReturn, "%s%s\n", toReturn, tempString);
        free(tempString);
    }

    //calendar events
    for(i = 0; i < getLength(obj->events); i++){
        tempString = printEvent(nextElement(&eventIterator));
        toReturn = realloc(toReturn, snprintf(NULL, 0, "%sBEGIN:VEVENT\n%sEND:VEVENT\n", toReturn, tempString) + 1);
        sprintf(toReturn, "%sBEGIN:VEVENT\n%sEND:VEVENT\n", toReturn, tempString);
        free(tempString);
    }
    toReturn = realloc(toReturn, snprintf(NULL, 0,  "%sEND:VCALENDAR\n", toReturn) + 1);
    sprintf(toReturn, "%sEND:VCALENDAR\n", toReturn);
    return toReturn;
}

char* printError(ICalErrorCode err){
    switch(err){
        case OK:
            return "OK";
        case INV_FILE:
            return "INVALID FILE";
        case INV_CAL:
            return "INVALID CALENDAR";
        case INV_VER:
            return "INVALID VERSION";
        case DUP_VER:
            return "DUPLICATE VERSION";
        case INV_PRODID:
            return "INVALID PRODUCT ID";
        case DUP_PRODID:
            return "DUPLICATE PRODUCT ID";
        case INV_EVENT:
            return "INVALID EVENT";
        case INV_DT:
            return "INVALID DATETIME";
        case INV_ALARM:
            return "INVALID ALARM";
        case WRITE_ERROR:
            return "WRITE ERROR";
        case OTHER_ERROR:
            return "OTHER ERROR";
        default:
            return "UNKNOWN ERROR";
    }
    return NULL;
}

ICalErrorCode writeCalendar(char* fileName, const Calendar* obj){
    FILE *fp;
    ListIterator propertyIterator, eventIterator, alarmIterator;
    Property *tempProperty;
    Event *tempEvent;
    Alarm *tempAlarm;
    DateTime tempDateTime;
    int i, j, k;

    if(fileName == NULL || obj == NULL || strcmp(strrchr(fileName, '.'), ".ics")) // validate arguments
        return WRITE_ERROR;

    fp = fopen(fileName, "w");

    propertyIterator = createIterator(obj->properties);
    eventIterator = createIterator(obj->events);
    fprintf(fp, "BEGIN:VCALENDAR\r\nVERSION:%.1f\r\nPRODID:%s\r\n", obj->version, obj->prodID);
    for(i = 0; i < getLength(obj->properties); i++){
        tempProperty = (Property*)nextElement(&propertyIterator);
        fprintf(fp, "%s:%s\r\n", tempProperty->propName, tempProperty->propDescr);
    }
    for(i = 0; i < getLength(obj->events); i++){
        tempEvent = (Event*)nextElement(&eventIterator);
        fprintf(fp, "BEGIN:VEVENT\r\n");
        fprintf(fp, "UID:%s\r\n", tempEvent->UID);
        tempDateTime = (DateTime)(tempEvent->startDateTime);
        if(tempDateTime.UTC)
            fprintf(fp, "DTSTART:%sT%sZ\r\n", tempDateTime.date, tempDateTime.time);
        else
            fprintf(fp, "DTSTART:%sT%s\r\n", tempDateTime.date, tempDateTime.time);
        tempDateTime = (DateTime)(tempEvent->creationDateTime);
        if(tempDateTime.UTC)
            fprintf(fp, "DTSTAMP:%sT%sZ\r\n", tempDateTime.date, tempDateTime.time);
        else
            fprintf(fp, "DTSTAMP:%sT%s\r\n", tempDateTime.date, tempDateTime.time);
        propertyIterator = createIterator(tempEvent->properties);
        for(j = 0; j < getLength(tempEvent->properties); j++){
            tempProperty = (Property*)nextElement(&propertyIterator);
            fprintf(fp, "%s:%s\r\n", tempProperty->propName, tempProperty->propDescr);
        }
        alarmIterator = createIterator(tempEvent->alarms);
        for(j = 0; j < getLength(tempEvent->alarms); j++){
            tempAlarm = (Alarm*)nextElement(&alarmIterator);
            fprintf(fp, "BEGIN:VALARM\r\n");
            fprintf(fp, "ACTION:%s\r\nTRIGGER:%s\r\n", tempAlarm->action, tempAlarm->trigger);
            propertyIterator = createIterator(tempAlarm->properties);
            for(k = 0; k < getLength(tempAlarm->properties); k++){
                tempProperty = (Property*)nextElement(&propertyIterator);
                fprintf(fp, "%s:%s\r\n", tempProperty->propName, tempProperty->propDescr);
            }
            fprintf(fp, "END:VALARM\r\n");
        }
        fprintf(fp, "END:VEVENT\r\n");
    }
    fprintf(fp, "END:VCALENDAR\r\n");
    fclose(fp);
    return OK;
}

bool searchList(char *list[], char *string, int length){
    int i;
    for(i = 0; i < length; i++){
        if(!strcmp(list[i], string)){
            return true;
        }
    }
    return false;
}

ICalErrorCode validateCalendar(const Calendar* obj){
    ListIterator propertyIterator, eventIterator, alarmIterator;
    Property *tempProperty;
    Event *tempEvent;
    Alarm *tempAlarm;
    DateTime tempDateTime;
    int i, j, k;

    bool invCal = false, invEvent = false, invAlarm = false;
    //CALENDAR
    bool calscale = false, method = false;
    //EVENT
    //char *eventPropOnlyOnceList[16] = {CLASS", "CREATED", "DESCRIPTION", "GEO", "LAST-MODIFIED", "LOCATION", "ORGANIZER", 
    //"PRIORITY", "SEQ", "STATUS", "SUMMARY", "TRANSP", "URL", "RECURID", "RRULE"};
    char *eventPropList[27] = {"CLASS", "CREATED", "DESCRIPTION", "GEO", "LAST-MODIFIED", "LOCATION", "ORGANIZER", 
    "PRIORITY", "SEQUENCE", "STATUS", "SUMMARY", "TRANSP", "URL", "RECURID", "RRULE", "DTEND", "DURATION", 
    "ATTACH", "ATTENDEE", "CATEGORIES", "COMMENT", "CONTACT", "EXDATE", "RSTATUS", "RELATED", "RESOURCES", "RDATE"};
    bool class, created, description, geo, last_modified, location, organizer, priority, sequence, status, summary, transp, url, recurid, rrule, dtend, eventDuration;
    //ALARM
    char *alarmPropList[4] = {"ACTION", "DURATION", "REPEAT", "ATTACH"};
    bool alarmDuration, repeat, attach;

    //validate obj
    if(obj == NULL)
        return INV_CAL;

    //validate Calendar version and prodid
    if(strlen(obj->prodID) > 1000 || strlen(obj->prodID) == 0)
        invCal = true;
    if(obj->events == NULL || obj->properties == NULL)
        return INV_CAL;
    //validate calendar properties
    propertyIterator = createIterator(obj->properties);
    eventIterator = createIterator(obj->events);
    for(i = 0; i < getLength(obj->properties); i++){
        tempProperty = (Property*)nextElement(&propertyIterator);
        if(strlen(tempProperty->propName) > 200 || strlen(tempProperty->propName) == 0)
            invCal = true;
        if(strlen(tempProperty->propDescr) == 0)
            invCal = true;
        if(!strcmp(tempProperty->propName, "BEGIN"))
            invCal = true;
        if(!strcmp(tempProperty->propName, "CALSCALE")){
            if(calscale)
                invCal = true;
            calscale = true;
        }
        if(!strcmp(tempProperty->propName, "METHOD")){
            if(method)
                invCal = true;
            method = true;
        }
        if(strcmp(tempProperty->propName, "CALSCALE") && strcmp(tempProperty->propName, "METHOD"))
            invCal = true;
    }
    //validate calendar events
    if(getLength(obj->events) < 1)
        invCal = true;
    for(i = 0; i < getLength(obj->events); i++){
        //validate event UID, DTSTAMP and DTSTART
        tempEvent = (Event*)nextElement(&eventIterator);
        class = false, created = false, description = false, geo = false, last_modified = false, location = false, organizer = false, priority = false, 
        sequence = false, status = false, summary = false, transp = false, url = false, recurid = false, rrule = false, dtend = false, eventDuration = false;
        if(tempEvent->alarms == NULL || tempEvent->properties == NULL){
            invEvent = true;
            continue;
        }
        if(strlen(tempEvent->UID) > 1000 || strlen(tempEvent->UID) == 0)
            invEvent = true;
        tempDateTime = (DateTime)(tempEvent->startDateTime);
        if(strlen(tempDateTime.date) != 8 || strlen(tempDateTime.date) == 0)
            invEvent = true;
        if(strlen(tempDateTime.time) != 6 || strlen(tempDateTime.time) == 0)
            invEvent = true;
        tempDateTime = (DateTime)(tempEvent->creationDateTime);
        if(strlen(tempDateTime.date) != 8 || strlen(tempDateTime.date) == 0)
            invEvent = true;
        if(strlen(tempDateTime.time) != 6  || strlen(tempDateTime.time) == 0)
            invEvent = true;
        //validate event properties
        propertyIterator = createIterator(tempEvent->properties);
        for(j = 0; j < getLength(tempEvent->properties); j++){
            tempProperty = (Property*)nextElement(&propertyIterator);
            if(strlen(tempProperty->propName) > 200 || strlen(tempProperty->propName) == 0)
                invEvent = true;
            if(strlen(tempProperty->propDescr) == 0)
                invEvent = true;
            // does not work since you cannot modify obj
            /*if(searchList(eventPropOnlyOnceList, tempProperty->propName, 16)){ //a property that may only occur once
                deleteDataFromList(tempEvent->properties, tempProperty);
                if(findElement(tempEvent->properties, &comparePropertyName, tempProperty->propName) != NULL) //if not null, it means that property already exists.
                    return INV_EVENT;
                insertBack(tempEvent->properties, tempEvent);
            }*/
            if(!strcmp(tempProperty->propName, "CLASS")){
                if(class)
                    invEvent = true;
                class = true;
            }
            if(!strcmp(tempProperty->propName, "CREATED")){
                if(created)
                    invEvent = true;
                created = true;
            }
            if(!strcmp(tempProperty->propName, "DESCRIPTION")){
                if(description)
                    invEvent = true;
                description = true;
            }
            if(!strcmp(tempProperty->propName, "GEO")){
                if(geo)
                    invEvent = true;
                geo = true;
            }
            if(!strcmp(tempProperty->propName, "LAST-MODIFIED")){
                if(last_modified)
                    invEvent = true;
                last_modified = true;
            }
            if(!strcmp(tempProperty->propName, "LOCATION")){
                if(location)
                    invEvent = true;
                location = true;
            }
            if(!strcmp(tempProperty->propName, "ORGANIZER")){
                if(organizer)
                    invEvent = true;
                organizer = true;
            }
            if(!strcmp(tempProperty->propName, "PRIORITY")){
                if(priority)
                    invEvent = true;
                priority = true;
            }
            if(!strcmp(tempProperty->propName, "SEQUENCE")){
                if(sequence)
                    invEvent = true;
                sequence = true;
            }
            if(!strcmp(tempProperty->propName, "STATUS")){
                if(status)
                    invEvent = true;
                status = true;
            }
            if(!strcmp(tempProperty->propName, "SUMMARY")){
                if(summary)
                    invEvent = true;
                summary = true;
            }
            if(!strcmp(tempProperty->propName, "TRANSP")){
                if(transp)
                    invEvent = true;
                transp = true;
            }
            if(!strcmp(tempProperty->propName, "URL")){
                if(url)
                    invEvent = true;
                url = true;
            }
            if(!strcmp(tempProperty->propName, "RECURID")){
                if(recurid)
                    invEvent = true;
                recurid = true;
            }
            if(!strcmp(tempProperty->propName, "RRULE")){
                if(rrule)
                    invEvent = true;
                rrule = true;
            }
            //only one of DTEND and DURATION can exist
            if(!strcmp(tempProperty->propName, "DTEND")){
                if(eventDuration)
                    invEvent = true;
                dtend = true;
            }
            if(!strcmp(tempProperty->propName, "DURATION")){
                if(dtend)
                    invEvent = true;
                eventDuration = true;
            }
            //serach for permitted optional prop
            if(!searchList(eventPropList, tempProperty->propName, 27))
                invEvent = true;
        }
        //validate alarms
        alarmIterator = createIterator(tempEvent->alarms);
        for(j = 0; j < getLength(tempEvent->alarms); j++){
            //validate alarm action and trigger
            tempAlarm = (Alarm*)nextElement(&alarmIterator);
            if(tempAlarm->properties == NULL){
                invAlarm = true;
                continue;
            }
            alarmDuration = false, repeat = false, attach = false;
            if(strlen(tempAlarm->action) > 200 || strlen(tempAlarm->action) == 0)
                invAlarm = true;
            if(tempAlarm->trigger == NULL || strlen(tempAlarm->trigger) == 0)
                invAlarm = true;
            propertyIterator = createIterator(tempAlarm->properties);
            for(k = 0; k < getLength(tempAlarm->properties); k++){
                tempProperty = (Property*)nextElement(&propertyIterator);
                if(strlen(tempProperty->propName) > 200 || strlen(tempProperty->propName) == 0)
                    invAlarm = true;
                if(strlen(tempProperty->propDescr) == 0)
                    invAlarm = true;
                //REPEAT, DURATION and ATTACH may only occur once
                if(!strcmp(tempProperty->propName, "REPEAT")){
                    if(repeat)
                        invAlarm = true;
                    repeat = true;
                }
                if(!strcmp(tempProperty->propName, "DURATION")){
                    if(alarmDuration)
                        invAlarm = true;
                    alarmDuration = true;
                }
                if(!strcmp(tempProperty->propName, "ATTACH")){
                    if(attach)
                        invAlarm = true;
                    attach = true;
                }
                //serach for permitted optional prop
                if(!searchList(alarmPropList, tempProperty->propName, 4))
                    invAlarm = true;
            }
            if((repeat == true && alarmDuration == false) || (repeat == false && alarmDuration == true)) //if one is true, so must the other
                invAlarm = true;
        }
    }
    // return by priority
    if(invCal)
        return INV_CAL;
    if(invEvent)
        return INV_EVENT;
    if(invAlarm)
        return INV_ALARM;
    return OK;
}

char* dtToJSON(DateTime prop){
    char *string;
    if(prop.UTC){
        string = malloc(snprintf(NULL, 0, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":true}", prop.date, prop.time) + 1);
        sprintf(string, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":true}", prop.date, prop.time);
    }
    else{
        string = malloc(snprintf(NULL, 0, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":false}", prop.date, prop.time) + 1);
        sprintf(string, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":false}", prop.date, prop.time);
    }
    return string;
}


char* eventToJSON(const Event* event){
    ListIterator iterator;
    Property *tempProp;
    char *string, *dt, *sumVal = NULL;
    int i;

    if(event == NULL){
        string = malloc(3);
        strcpy(string, "{}");
        return string;
    }

    //find SUMMARY DISCRIPTION
    iterator = createIterator(event->properties);
    for(i = 0; i < getLength(event->properties); i++){
        tempProp = nextElement(&iterator);
        if(!strcmp(tempProp->propName, "SUMMARY")){
            sumVal = strdup(tempProp->propDescr);
            break;
        }
    }
    //check if SUMMARY is not found
    if(sumVal == NULL)
        sumVal = strdup("");
    
    dt = dtToJSON(event->startDateTime);
    string = malloc(snprintf(NULL, 0, "{\"startDT\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"%s\"}", dt, 3 + getLength(event->properties), getLength(event->alarms), sumVal) + 1);
    sprintf(string, "{\"startDT\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"%s\"}", dt, 3 + getLength(event->properties), getLength(event->alarms), sumVal);
    free(dt);
    free(sumVal);
    return string;
}


char* eventListToJSON(const List* eventList){
    ListIterator iterator;
    List *tempEventList;
    Event *tempEvent;
    char *string, *tempString;
    int i;

    tempEventList = (List *)eventList;
    if(eventList == NULL || getLength(tempEventList) == 0)
        return strdup("[]");
    
    string = strdup("[");
    iterator = createIterator(tempEventList);
    for(i = 0; i < getLength(tempEventList); i++){
        tempEvent = nextElement(&iterator);
        tempString = eventToJSON(tempEvent);
        if(i == getLength(tempEventList) - 1){
            string = realloc(string, snprintf(NULL, 0, "%s%s]", string, tempString) + 1);
            sprintf(string, "%s%s]", string, tempString);
        }
        else{
            string = realloc(string, snprintf(NULL, 0, "%s%s,", string, tempString) + 1);
            sprintf(string, "%s%s,", string, tempString);
        }
    }
    return string;
}

char* alarmToJSON(const Alarm* alarm){
    char *string;

    if(alarm == NULL){
        string = malloc(3);
        strcpy(string, "{}");
        return string;
    }
    
    string = malloc(snprintf(NULL, 0, "{\"action\":\"%s\",\"trigger\":\"%s\",\"numProps\":%d}", alarm->action, alarm->trigger, getLength(alarm->properties)) + 1);
    sprintf(string, "{\"action\":\"%s\",\"trigger\":\"%s\",\"numProps\":%d}", alarm->action, alarm->trigger, getLength(alarm->properties));
    return string;
}

char* alarmListToJSON(const List* alarmList){
    ListIterator iterator;
    List *tempAlarmList;
    Alarm *tempAlarm;
    char *string, *tempString;
    int i;

    tempAlarmList = (List *)alarmList;
    if(alarmList == NULL || getLength(tempAlarmList) == 0)
        return strdup("[]");
    
    string = strdup("[");
    iterator = createIterator(tempAlarmList);
    for(i = 0; i < getLength(tempAlarmList); i++){
        tempAlarm = nextElement(&iterator);
        tempString = alarmToJSON(tempAlarm);
        if(i == getLength(tempAlarmList) - 1){
            string = realloc(string, snprintf(NULL, 0, "%s%s]", string, tempString) + 1);
            sprintf(string, "%s%s]", string, tempString);
        }
        else{
            string = realloc(string, snprintf(NULL, 0, "%s%s,", string, tempString) + 1);
            sprintf(string, "%s%s,", string, tempString);
        }
    }
    return string;
}

char* propToJSON(const Property* prop){
    char *string;

    if(prop == NULL){
        string = malloc(3);
        strcpy(string, "{}");
        return string;
    }
    
    string = malloc(snprintf(NULL, 0, "\"%s\":\"%s\"", prop->propName, prop->propDescr) + 1);
    sprintf(string, "\"%s\":\"%s\"", prop->propName, prop->propDescr);
    return string;
}

char* propListToJSON(const List* propList){
    ListIterator iterator;
    List *tempPropList;
    Property *tempProp;
    char *string, *tempString;
    int i;

    tempPropList = (List *)propList;
    if(propList == NULL || getLength(tempPropList) == 0)
        return strdup("{}");
    
    string = strdup("{");
    iterator = createIterator(tempPropList);
    for(i = 0; i < getLength(tempPropList); i++){
        tempProp = nextElement(&iterator);
        tempString = propToJSON(tempProp);
        if(i == getLength(tempPropList) - 1){
            string = realloc(string, snprintf(NULL, 0, "%s, %s}", string, tempString) + 1);
            sprintf(string, "%s, %s}", string, tempString);
        }
        else{
            string = realloc(string, snprintf(NULL, 0, "%s, %s", string, tempString) + 1);
            sprintf(string, "%s, %s", string, tempString);
        }
    }
    return string;
}

char* calendarToJSON(const Calendar* cal){
    char *string;
    
    if(cal == NULL)
        return strdup("{}");
    
    string = malloc(snprintf(NULL, 0, "{\"version\":%d,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d}", (int)cal->version, cal->prodID, 2 + getLength(cal->properties), getLength(cal->events)) + 1);
    sprintf(string, "{\"version\":%d,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d}", (int)cal->version, cal->prodID, 2 + getLength(cal->properties), getLength(cal->events));
    
    return string;
}

Calendar* JSONtoCalendar(const char* str){
    Calendar *cal;
    char *temp;
    
    if(str == NULL)
        return NULL;

    cal = malloc(sizeof(Calendar));
    cal->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
    cal->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    
    temp = strdup(str);
    if(strtok(temp, ":") == NULL)
        return NULL;
    cal->version = atof(strtok(NULL, ","));
    if(strtok(NULL, ":") == NULL)
        return NULL;
    strcpy(cal->prodID, strtok(NULL, "\""));
    free(temp);

    return cal;
}

Event* JSONtoEvent(const char* str){
    Event *event;
    char *temp;

    if(str == NULL)
        return NULL;

    event = malloc(sizeof(Event));
    event->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
    event->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);

    temp = strdup(str);
    if(strtok(temp, ":") == NULL)
        return NULL;
    strcpy(event->UID, strtok(NULL, "\""));
    free(temp);

    return event;
}

void addEvent(Calendar* cal, Event* toBeAdded){
    if(cal != NULL && toBeAdded != NULL)
        insertBack(cal->events, toBeAdded);
}

/*HELPER FUNCTIONS THAT MUST BE IMPLEMENTED*/
void deleteEvent(void* toBeDeleted){
    Event* tempEvent;
    if(toBeDeleted == NULL)
        return;
    tempEvent = (Event*)toBeDeleted;
    freeList(tempEvent->properties);
    freeList(tempEvent->alarms);
    free(tempEvent);
}

int compareEvents(const void* first, const void* second){
    Event *tempEvent1, *tempEvent2;
    char *t1, *t2;
    int result;
    if(first == NULL || second == NULL)
        return 0;
    tempEvent1 = (Event*)first;
    tempEvent2 = (Event*)second;
    t1 = printEvent(tempEvent1);
    t2 = printEvent(tempEvent2);
    result = strcmp(t1, t2);
    free(t1);
    free(t2);
    return result;
}

char* printEvent(void* toBePrinted){
    Event *tempEvent;
    ListIterator propertyIterator, alarmIterator;
    char *toReturn, *tempString;
    int i;

    if(toBePrinted == NULL)
        return NULL;
    
    tempEvent = (Event *)toBePrinted;
    propertyIterator = createIterator(tempEvent->properties);
    
    // print event info
    toReturn = malloc(snprintf(NULL, 0, "UID:%s\n", tempEvent->UID) + 1);    
    sprintf(toReturn, "UID:%s\n", tempEvent->UID);
    tempString = printDate(tempEvent->startDateTime.date);
    toReturn = realloc(toReturn, snprintf(NULL, 0, "%sDTSTART:%s\n", toReturn, tempString) + 1);
    sprintf(toReturn, "%sDTSTART:%s\n", toReturn, tempString);
    free(tempString);
    tempString = printDate(tempEvent->creationDateTime.date);
    toReturn = realloc(toReturn, snprintf(NULL, 0, "%sDTSTAMP:%s\n", toReturn, tempString) + 1);
    sprintf(toReturn, "%sDTSTAMP:%s\n", toReturn, tempString);
    free(tempString);
    // print events properties
    for(i = 0; i < getLength(tempEvent->properties); i++){
        tempString = printProperty(nextElement(&propertyIterator));    
        toReturn = realloc(toReturn, snprintf(NULL, 0, "%s%s\n", toReturn, tempString) + 1);
        sprintf(toReturn, "%s%s\n", toReturn, tempString);
        free(tempString);
    }

    // print alarm properties
    alarmIterator = createIterator(tempEvent->alarms);
    for(i = 0; i < getLength(tempEvent->alarms); i++){
        tempString = printAlarm(nextElement(&alarmIterator));
        toReturn = realloc(toReturn, snprintf(NULL, 0, "%s%s", toReturn, tempString) + 1);
        sprintf(toReturn, "%s%s", toReturn, tempString);
        free(tempString);
    }
    return toReturn;
}

void deleteAlarm(void* toBeDeleted){
    Alarm* tempAlarm;
    if(toBeDeleted == NULL)
        return;
    tempAlarm = (Alarm*)toBeDeleted;
    freeList(tempAlarm->properties);
    free(tempAlarm->trigger);
    free(tempAlarm);
}

int compareAlarms(const void* first, const void* second){
    Alarm *tempAlarm1, *tempAlarm2;
    char *t1, *t2;
    int result;
    if(first == NULL || second == NULL)
        return 0;
    tempAlarm1 = (Alarm*)first;
    tempAlarm2 = (Alarm*)second;
    t1 = printEvent(tempAlarm1);
    t2 = printEvent(tempAlarm2);
    result = strcmp(t1, t2);
    free(t1);
    free(t2);
    return result;
}

char* printAlarm(void* toBePrinted){
    Alarm *tempAlarm;
    ListIterator propertyIterator;
    char *toReturn, *tempString;
    int i;

    if(toBePrinted == NULL)
        return NULL;

    tempAlarm = (Alarm *)toBePrinted;
    toReturn = malloc(snprintf(NULL, 0, "BEGIN:VALARM\nACTION:%s\nTRIGGER:%s\n", tempAlarm->action, tempAlarm->trigger) + 1);
    sprintf(toReturn, "BEGIN:VALARM\nACTION:%s\nTRIGGER:%s\n", tempAlarm->action, tempAlarm->trigger);
    propertyIterator = createIterator(tempAlarm->properties);
    for(i = 0; i < getLength(tempAlarm->properties); i++){
        tempString = printProperty(nextElement(&propertyIterator));
        toReturn = realloc(toReturn, snprintf(NULL, 0, "%s%s\n", toReturn, tempString) + 1);
        sprintf(toReturn, "%s%s\n", toReturn, tempString);
        free(tempString);
    }
    toReturn = realloc(toReturn, snprintf(NULL, 0, "%sEND:VALARM\n", toReturn) + 1);
    sprintf(toReturn, "%sEND:VALARM\n", toReturn);
    return toReturn;
}

void deleteProperty(void* toBeDeleted){
    Property* tempProp;
    if(toBeDeleted == NULL)
        return;
    tempProp = (Property*)toBeDeleted;
    free(tempProp);
}

int compareProperties(const void* first, const void* second){
    Property *tempProp1, *tempProp2;
    char *t1, *t2;
    int result;
    if(first == NULL || second == NULL)
        return 0;
    tempProp1 = (Property*)first;
    tempProp2 = (Property*)second;
    t1 = printProperty(tempProp1);
    t2 = printProperty(tempProp2);
    result = strcmp(t1, t2);
    free(t1);
    free(t2);
    return result;
}

bool comparePropertyName(const void* first, const void* second){
    Property *tempProp;
    if(first == NULL || second == NULL)
        return 0;
    tempProp = (Property*)first;
    if(!strcmp(tempProp->propName, second))
        return true;
    return false;
}

char* printProperty(void* toBePrinted){
    Property *tempProp;
    char *toReturn;
    if(toBePrinted == NULL)
        return NULL;
    tempProp = (Property *)toBePrinted;
    toReturn = malloc(snprintf(NULL, 0, "%s:%s", tempProp->propName, tempProp->propDescr) + 1);
    sprintf(toReturn, "%s:%s", tempProp->propName, tempProp->propDescr);
    return toReturn;
}

void deleteDate(void* toBeDeleted){
    DateTime* tempDateTime;
    if(toBeDeleted == NULL)
        return;
    tempDateTime = (DateTime *)toBeDeleted;
    free(tempDateTime);
}

int compareDates(const void* first, const void* second){
    DateTime *tempDateTime1, *tempDateTime2;
    char *t1, *t2;
    int result;
    if(first == NULL || second == NULL)
        return 0;
    tempDateTime1 = (DateTime*)first;
    tempDateTime2 = (DateTime*)second;
    t1 = printEvent(tempDateTime1);
    t2 = printEvent(tempDateTime2);
    result = strcmp(t1, t2);
    free(t1);
    free(t2);
    return result;
}

char* printDate(void* toBePrinted){
    DateTime *tempDateTime;
    char *toReturn;
    if(toBePrinted == NULL)
        return NULL;
    tempDateTime = (DateTime*)toBePrinted;
    if(tempDateTime->UTC){
        toReturn = malloc(snprintf(NULL, 0, "%sT%sZ", tempDateTime->date, tempDateTime->time) + 1);
        sprintf(toReturn, "%sT%sZ", tempDateTime->date, tempDateTime->time);
    }
    else{
        toReturn = malloc(snprintf(NULL, 0, "%sT%s", tempDateTime->date, tempDateTime->time) + 1);
        sprintf(toReturn, "%sT%s", tempDateTime->date, tempDateTime->time);
    }
    return toReturn;
}

char *createError_json(char *error){
    char *string;
    string = malloc(snprintf(NULL, 0, "{\"error\":\"%s\"}", error) + 1);
    sprintf(string, "{\"error\":\"%s\"}", error);
    return string;
}

char *json_from_createCalendar(char *filename){
    Calendar *obj;
    char * temp;
    ICalErrorCode code = createCalendar(filename, &obj);
    if(code == OK){
        code = validateCalendar(obj);
        if(code == OK){
            temp = calendarToJSON(obj);
            deleteCalendar(obj);
            return temp;
        }
        else{
            deleteCalendar(obj);
            return createError_json(printError(code));
        }
    }
    return createError_json(printError(code));
}

char *json_from_eventList(char *filename){
    Calendar *obj;
    char *temp;
    ICalErrorCode code = createCalendar(filename, &obj);
    if(code == OK){
        code = validateCalendar(obj);
        if(code == OK){
            temp = eventListToJSON(obj->events);
            deleteCalendar(obj);
            return temp;
        }
        else{
            deleteCalendar(obj);
            return createError_json(printError(code));
        }
    }
    return createError_json(printError(code));
}

char *json_from_alarmList(char *filename, int eventNo){
    Calendar *obj;
    ListIterator eventIterator;
    Event *tempEvent;
    char *temp;

    ICalErrorCode code = createCalendar(filename, &obj);
    if(code == OK){
        code = validateCalendar(obj);
        if(code == OK){
            eventIterator = createIterator(obj->events);
            for(int i = 0; i < getLength(obj->events); i++){
                tempEvent = nextElement(&eventIterator);
                if(i + 1 == eventNo){
                    temp = alarmListToJSON(tempEvent->alarms);
                    deleteCalendar(obj);
                    return temp;
                }
            }
            deleteCalendar(obj);
            return createError_json("INV_EVENTNO");
        }
        else{
            deleteCalendar(obj);
            return createError_json(printError(code));
        }
    }
    return createError_json(printError(code));
}

char *json_from_eventPropList(char *filename, int eventNo){
    Calendar *obj;
    ListIterator eventIterator;
    Event *tempEvent;
    char *temp;

    ICalErrorCode code = createCalendar(filename, &obj);
    if(code == OK){
        code = validateCalendar(obj);
        if(code == OK){
            eventIterator = createIterator(obj->events);
            for(int i = 0; i < getLength(obj->events); i++){
                tempEvent = nextElement(&eventIterator);
                if(i + 1 == eventNo){
                    temp = propListToJSON(tempEvent->properties);
                    deleteCalendar(obj);
                    return temp;
                }
            }
            deleteCalendar(obj);
            return createError_json("INV_EVENTNO");
        }
        else{
            deleteCalendar(obj);
            return createError_json(printError(code));
        }
    }
    return createError_json(printError(code));
}

char *calendar_from_json(char *filename, char *dtSTAMP, char *dtSTART, char *calJSON, char *eventJSON){
    char *string = malloc(snprintf(NULL, 0, "./uploads/%s", filename) + 1);
    sprintf(string, "./uploads/%s", filename);
    ICalErrorCode code;
    Calendar *obj = JSONtoCalendar(calJSON);
    Event *event = JSONtoEvent(eventJSON);
    strcpy(event->creationDateTime.date, dtSTAMP);
    strcpy(event->creationDateTime.time, "000000");
    event->creationDateTime.UTC = false;
    strcpy(event->startDateTime.date, dtSTART);
    strcpy(event->startDateTime.time, "000000");
    event->startDateTime.UTC = false;
    addEvent(obj, event);
    code = validateCalendar(obj);
    if(code == OK){
        writeCalendar(string, obj);
    }
    deleteCalendar(obj);
    return createError_json(printError(code));
}

char *addEventToFile(char *filename, char *dtSTAMP, char *dtSTART, char *eventJSON){
    ICalErrorCode code;
    Calendar *obj;
    char *string = malloc(snprintf(NULL, 0, "./uploads/%s", filename) + 1);
    sprintf(string, "./uploads/%s", filename);
    createCalendar(string, &obj);
    Event *event = JSONtoEvent(eventJSON);
    strcpy(event->creationDateTime.date, dtSTAMP);
    strcpy(event->creationDateTime.time, "000000");
    event->creationDateTime.UTC = false;
    strcpy(event->startDateTime.date, dtSTART);
    strcpy(event->startDateTime.time, "000000");
    event->startDateTime.UTC = false;
    addEvent(obj, event);
    code = validateCalendar(obj);
    if(code == OK){
        printf("OK\n");
        writeCalendar(string, obj);
    }
    deleteCalendar(obj);
    return createError_json(printError(code));
}