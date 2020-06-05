
var client = mqtt.connect('mqtts://fistbumper:bumpitup@broker.shiftr.io', {
  clientId: 'javascript'
});

var bumpTopic = "/fistbump";
var bumpMessage = "bump";

var queryTopic = "/bumpquery";
var replyTopic = "/bumpreply";

client.on('connect', function(){
  console.log('client has connected!');

  client.subscribe('/bumpreply');
  // client.unsubscribe('/example');

  setInterval(function(){
    sendQuery("areyouthere");
  }, 2000);
});

client.on('message', function(topic, message) {
  console.log('new message:', topic, message.toString());
});

$('#send_bump').on('click', function (e) {
    sendBump();
})

function sendBump()
{
    console.log("Sending bump");
    client.publish(bumpTopic, bumpMessage);
}

function sendQuery(message)
{
    console.log("Sending query - " + message);
    client.publish(queryTopic, message);
}
