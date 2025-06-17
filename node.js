const net = require('net');

const client = new net.Socket();

const ip = '127.0.0.1';
const port = 51234;

const messages = [
  "POST / HTTP/1.1\r\n" +
  "Host: 127.0.0.1\r\n" +
  "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36\r\n" +
  "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n" +
  "Accept-Language: en-US,en;q=0.5\r\n" +
  "Accept-Encoding: gzip, deflate, br\r\n" +
  "Connection: keep-alive\r\n" +
  "Content-Length: 20\r\n" +
  "Content-Type: text/plain\r\n" +
  "Cache-Control: no-cache\r\n" +
  "Pragma: no-cache\r\n" +
  "Origin: http://127.0.0.1\r\n" +
  "Sec-Fetch-Dest: document\r\n" +
  "Sec-Fetch-Mode: navigate\r\n" +
  "Sec-Fetch-Site: same-origin\r\n" +
  "Sec-Fetch-User: ?1\r\n" +
  "Upgrade-Insecure-Requests: 1\r\n" +
  "\r\n" +
  "hello nigger yes yes",

  // Можно добавить больше сообщений сюда
  "POST / HTTP/1.1\r\nHost: 127.0.0.1\r\nContent-Length: 11\r\n\r\nhello again",

  "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
];

client.connect(port, ip, () => {
  let currentMessageIndex = 0;
  let pos = 0;
  const chunkSize = 60;

  function sendChunk() {
    if (currentMessageIndex >= messages.length) {
      console.log("Все сообщения отправлены.");
      client.end();
      return;
    }

    const message = messages[currentMessageIndex];

    if (pos < message.length) {
      const chunk = message.slice(pos, pos + chunkSize);
      client.write(chunk);
      console.log(`Sent chunk from message #${currentMessageIndex + 1}: "${chunk}"`);
      pos += chunkSize;
      setTimeout(sendChunk, 200);
    } else {
      console.log(`Message #${currentMessageIndex + 1} полностью отправлено.`);
      pos = 0;
      currentMessageIndex++;
      setTimeout(sendChunk, 200);
    }
  }

  sendChunk();
});

client.on('data', (data) => {
  console.log('Received:', data.toString());
});

client.on('close', () => {
  console.log('Connection closed');
});
