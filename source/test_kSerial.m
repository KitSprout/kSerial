
clc, clear, close all;

s = kSerial(115200, 'clear');
s.setRecordBufferSize(1024 * 64);
s.open();

dd = 0.1;

fprintf('\n');

% {
fprintf('[     %02i     ]\n', 1);
info = s.packetSend();
fprintf('[W] [K, S, %04i, %s, %02X, %02X, %02X, 13]\n', info(1), s.typeConv(info(2)), info(3:5));
s.delay(dd);
[~, packetInfo, ~] = s.packetRecv();
if ~isempty(packetInfo)
    fprintf('[R] [K, S, %04i, %s, %02X, %02X, %02X, 13]\n', packetInfo(1), s.typeConv(packetInfo(2)), packetInfo(3:5));
else
    fprintf('[R] [K, S, [], 13, 10]')
end
fprintf('\n');
%}

%{
fprintf('[     %02i     ]\n', 2);
parameter = uint8([128, 10]);
info = s.packetSend(parameter);
fprintf('[W] [K, S, %04i, %s, %02X, %02X, %02X, 13]\n', info(1), s.typeConv(info(2)), info(3:5));
s.delay(dd);
[~, packetInfo, ~] = s.packetRecv();
if ~isempty(packetInfo)
    fprintf('[R] [K, S, %04i, %s, %02X, %02X, %02X, 13]\n', packetInfo(1), s.typeConv(packetInfo(2)), packetInfo(3:5));
else
    fprintf('[R] [K, S, [], 13, 10]');
end
fprintf('\n');
%}

%{
fprintf('[     %02i     ]\n', 3);
parameter = uint8([128, 10]);
data = uint8([1:255, 1:255, 1:255, 1:250]);
[info, send] = s.packetSend(parameter, data);
fprintf('[W] [K, S, %04i, %s, %02X, %02X, %02X', info(1), s.typeConv(info(2)), info(3:5));
for i = 1 : size(data, 2)
    fprintf(', %03i', send(7+i));
end
fprintf(', 13]\n');
s.delay(1);
[packetData, packetInfo, ~] = s.packetRecv();
if ~isempty(packetInfo)
    fprintf('[R] [K, S, %04i, %s, %02X, %02X, %02X', packetInfo(1), s.typeConv(packetInfo(2)), packetInfo(3:5));
    for i = 1 : size(packetData, 1)
        fprintf(', %03i', packetData(i))
    end
    fprintf(', 13]\n')
else
    fprintf('[R] [K, S, [], 13, 10]')
end
fprintf('\n');
%}

s.close();
