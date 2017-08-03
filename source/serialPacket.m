clear;

s = kSerial(115200, 'clear');
s.setRecordBufferSize(1024 * 16);
s.setRecvThreshold(0);
s.open();

% {
for i = 1 : 10000
    [packetData, packetInfo, packetLens] = s.packetRecv();
    if ~isempty(packetLens) && packetLens > 0
        sec   = s.record.data(1, end);
        msc   = s.record.data(2, end);
        count = s.record.data(3, end);
        tt    = s.getTime([1, 2], 0, 0.001);
        freq  = s.getFreq([1, 2], 256, 0.001);
        fprintf('[%02i:%02i:%02i][%4.0fHz] [command] 0x%02X [%05i]\n', tt(1), tt(2), fix(tt(3) / 10), freq, s.record.info(2, end), count);

% freq = s.getFreq(1);
% fprintf('[%4.0fHz]\n', freq);
    end
end
%}
s.close();

%{
% check lost packet
dd = s.record.data(3, end - s.record.count + 1 : end);
plot(1 : s.record.count - 1, dd(2 : end) - dd(1 : end - 1));
%}

% s.save2mat('rawData', {'gyr(1:3)', 'acc(4:6)', 'mag(7:9)', 'q((10:13))', 'sec(14)', 'msc(15)'});
