clear;

s = kSerial(115200, 'clear');
s.setRecordBufferSize(1024 * 16);
s.setRecvThreshold(0);
s.open();

dd = 0.1;

fprintf('\n')
% {
fprintf('[     %02i     ]\n', 1);
fprintf('[W] [K, S, %03i, %s, %03i, %03i, 13, 10]\n', 8, 'null', 0, 0)
s.packetSend();
s.delay(dd);
[~, packetInfo, ~] = s.packetRecv();
fprintf('[R] [K, S, %03i, %s, %03i, %03i, 13, 10]\n', packetInfo(1), s.typeConv(packetInfo(2)), packetInfo(3), packetInfo(4))
fprintf('\n')
%}
% {
fprintf('[     %02i     ]\n', 2);
parameter = uint8([128, 10]);
fprintf('[W] [K, S, %03i, %s, %03i, %03i, 13, 10]\n', 8, 'null', parameter(1), parameter(2))
s.packetSend(parameter);
s.delay(dd);
[~, packetInfo, ~] = s.packetRecv();
fprintf('[R] [K, S, %03i, %s, %03i, %03i, 13, 10]\n', packetInfo(1), s.typeConv(packetInfo(2)), packetInfo(3), packetInfo(4))
fprintf('\n')
%}
% {
fprintf('[     %02i     ]\n', 3);
parameter = uint8([128, 10]);
data = uint8(1:16);
fprintf('[W] [K, S, %03i, %s, %03i, %03i', 8 + size(data, 2) * s.typeSize(class(data)), class(data), parameter(1), parameter(2))
for i = 1 : size(data, 2)
    fprintf(', %03i', data(i))
end
fprintf(', 13, 10]\n')
s.packetSend(parameter, data);
s.delay(dd);
[packetData, packetInfo, ~] = s.packetRecv();
fprintf('[R] [K, S, %03i, %s, %03i, %03i', packetInfo(1), s.typeConv(packetInfo(2)), packetInfo(3), packetInfo(4))
for i = 1 : size(packetData, 1)
    fprintf(', %03i', packetData(i))
end
fprintf(', 13, 10]\n')
fprintf('\n')
%}

s.close();
