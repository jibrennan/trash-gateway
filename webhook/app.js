// proof of concept made by Jason Brennan on August 20th 2022, do not use for proudction.
const express    = require("express");
const bodyParser = require("body-parser");
const https      = require("https");
const mqtt       = require("mqtt");
//app items
const app = express();
app.use(bodyParser.urlencoded({extended: true}));

// create the MQTT client instance
const client = mqtt.connect('mqtt://127.0.0.1');  // TODO: read from config file instead, use mdns name
// after connecting to the Broker, do this:
client.on('connect', function () {
      console.log("MQTT client successfully registered.");
});
// subscribe to messages if you want.
client.on('message', function (topic, message) {
  // message is Buffer
  console.log("Received from broker: " + message.toString());
  // client.end();
});

// page requests:
app.get("/", function (req, res){
res. sendFile(__dirname+"/index.html"); // If a browser tries to access this gateway, send the html file as the response to the request.
});// end get page request
// post requests and actions:
app.post("/", function(req, response) {
    response.sendStatus(200); // be nice and send an OK back to the server
    req.on("data", function (data){ // once we get a Webhook from the service, we need to parse it...
      const IncomingData = JSON.parse(data); // creat an object that holds the service data we asked for. In this case the structure is in JSON format
      //console.log(IncomingData); // for debugging
      const Keyword = IncomingData.keywords; // see the webhook-info file for an example of what is contained
      const Phrase = IncomingData.text; // see the webhook-info file for an example of what is contained
      console.log(Keyword); // for debugging
      console.log(Phrase);  // for debugging
      // insert the MQTT publish stuff here
      client.publish('dev/test', 'webhook:' + Keyword + ' : ' + Phrase); // publish the message to the broker
  });// end of req.on

});// end of post

// set the page to listen to port:
app.listen(3000, function(){
  console.log("server is running on port 3000");
});// end listen
