// Copyright 2019 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

// This example does the same function as the "basic" example, but demonstrates how easy it is
// to use the Notecard libraries to construct JSON commands and also to extract responses.
//
// Using the Notecard library, you can also easily set up your Arduino environment to "watch"
// JSON request and response traffic going to/from the Notecard on your Arduino debug port.
//
// Note that by using the Notecard library, it is also quite easy to connect the Notecard to
// a Microcontroller's I2C ports (SDA and SCL) rather than using Serial, in case there is no
// unused serial port available to use for the Notecard.

#include "main.h"
#include "event.h"
#include "note.h"

// This is the unique Product Identifier for your device.  This Product ID tells the Notecard what
// type of device has embedded the Notecard, and by extension which vendor or customer is in charge
// of "managing" it.  In order to set this value, you must first register with notehub.io and
// "claim" a unique product ID for your device.  It could be something as simple as as your email
// address in reverse, such as "com.gmail.smith.lisa.test-device" or "com.outlook.gates.bill.demo"

#define myProductID "org.coca-cola.soda.vending-machine.v2"
#define myLiveDemo  true

// The amount of time between loops that take samples and send them to the service
#if myLiveDemo
#define DELAY_PERIOD    (15*1000)       // 15 seconds
#else
#define DELAY_PERIOD    (15*60*1000)    // 15 minutes
#endif

// Events to wait for within our loop that could trigger an update
#ifdef EVENT_BUTTON
#define EVENTS_TO_WAIT_FOR  EVENT_BUTTON
#else
#define EVENTS_TO_WAIT_FOR  0
#endif

// One-time initialization
void setup() {

	// "NoteNewRequest()" uses the bundled "J" json package to allocate a "req", which is a JSON object
	// for the request to which we will then add Request arguments.  The function allocates a "req"
	// request structure using malloc() and initializes its "req" field with the type of request.
	J *req = NoteNewRequest("service.set");

	// This command (required) causes the data to be delivered to the Project on notehub.io that has claimed
	// this Product ID.  (see above)
	JAddStringToObject(req, "product", myProductID);

	// This command determines how often the Notecard connects to the service.  If "continuous" the Notecard
    // immediately establishes a session with the service at notehub.io, and keeps it active continuously.
    // Because of the power requirements of a continuous connection, a battery powered device would instead
    // only sample its sensors occasionally, and would only upload to the service on a periodic basis.
#if myLiveDemo
	JAddStringToObject(req, "mode", "continuous");
#else
	JAddStringToObject(req, "mode", "periodic");
	JAddNumberToObject(req, "minutes", 60);
#endif

	// Issue the request, telling the Notecard how and how often to access the service.
	// This results in a JSON message to Notecard formatted like:
	//	   { "req"	   : "service.set",
	//		 "product" : myProductID,
	//		 "mode"	   : "continuous"
	//	   }
	// Note that NoteRequest() always uses free() to release the request data structure, and it
	// returns "true" if success and "false" if there is any failure.
	NoteRequest(req);

}

// This  main loop which is called repeatedly, add outbound data every 15 seconds
void loop() {

	// Simulate an event counter of some kind
	static unsigned eventCounter = 0;
	eventCounter = eventCounter + 1;

	// Rather than simulating a temperature reading, use a Notecard request to read the temp
	// from the Notecard's built-in temperature sensor.  We use NoteRequestResponse() to indicate
	// that we would like to examine the response of the transaction.  This method takes a "request" JSON
	// data structure as input, then processes it and returns a "response" JSON data structure with
	// the response.  Note that because the Notecard library uses malloc(), developers must always
	// check for NULL to ensure that there was enough memory available on the microcontroller to
	// satisfy the allocation request.
	double temperature = 0;
    J *rsp = NoteRequestResponse(NoteNewRequest("card.temp"));
    if (rsp != NULL) {
        temperature = JGetNumber(rsp, "value");
        NoteDeleteResponse(rsp);
    }

	// Do the same to retrieve the voltage that is detected by the Notecard on its V+ pin.
	double voltage = 0;
    rsp = NoteRequestResponse(NoteNewRequest("card.voltage"));
    if (rsp != NULL) {
        voltage = JGetNumber(rsp, "value");
        NoteDeleteResponse(rsp);
    }

	// Enqueue the measurement to the Notecard for transmission to the Notehub, adding the "start"
	// flag for demonstration purposes to upload the data instantaneously, so that if you are looking
	// at this on notehub.io you will see the data appearing 'live'.)
    J *req = NoteNewRequest("note.add");
	if (req != NULL) {
	    JAddStringToObject(req, "file", "sensors.qo");
	    JAddBoolToObject(req, "start", true);
		J *body = JCreateObject();
		if (body != NULL) {
			JAddNumberToObject(body, "temp", temperature);
			JAddNumberToObject(body, "voltage", voltage);
			JAddNumberToObject(body, "count", eventCounter);
#ifdef EVENT_BUTTON
            if ((eventOccurred() & EVENT_BUTTON) != 0) {
                JAddBoolToObject(body, "button", true);
                eventClear(EVENT_BUTTON);
            }
#endif
		    JAddItemToObject(req, "body", body);
		}
	    NoteRequest(req);
	}

	// Delay between measurements
#if EVENTS
    eventWait(EVENTS_TO_WAIT_FOR, DELAY_PERIOD);
#else
	delay(DELAY_PERIOD);
#endif

}
