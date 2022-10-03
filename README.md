# demo3

udp 테스트

https://learn.microsoft.com/ko-kr/windows/win32/winsock/windows-sockets-error-codes-2
https://www.slideshare.net/sm9kr/windows-registered-io-rio 
http://www.serverframework.com/asynchronousevents/rio/ 
https://blog.grijjy.com/2018/08/29/creating-high-performance-udp-servers-on-windows-and-linux/

note:<br/>  
zero_byte_read 쓰려면 rio는 소용이 없을것 같음

tcp는 데이터를 스트림으로 다루기 때문에 소켓버퍼에 데이터가 끊긴채로 들어오는것을 처리해 줘야함
udp는 보내면 보낸만큼 데이터그램이 받아지지만 반드시 도착한다고 보장하지 않음