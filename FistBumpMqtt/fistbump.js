
var client = mqtt.connect('mqtt://fistbumper:bumpitup@broker.shiftr.io', {
  clientId: 'javascript'
});

var bumpTopic = "/fistbump";
var bumpMessage = "bump";

client.on('connect', function(){
  console.log('client has connected!');

  client.subscribe('/example');
  // client.unsubscribe('/example');

  setInterval(function(){
    client.publish('/hello', 'world');
  }, 1000);
});

client.on('message', function(topic, message) {
  console.log('new message:', topic, message.toString());
});

$('#send_bump').on('click', function (e) {
    console.log("Sending bump");
    client.publish(bumpTopic, bumpMessage);
})

