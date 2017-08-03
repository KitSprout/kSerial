clear;

s = kSerial(115200, 'clear');
s.setRecordBufferSize(1024 * 16);
s.open();

fig = figure(1);
set(fig, 'Position', [100, 140, 1200, 600], 'color', 'w');
subFig = subplot(1, 1, 1);

osc = kSerialOscilloscope();
osc.setWindow(2^14 * [1, -1], 1000);
osc.curveChannel = 1 : 3;
osc.curveColor   = ['r', 'g', 'b'];
osc.initOscilloscope(subFig, 'runtime', 'data');

while ishandle(osc.fig)
    [packetData, packetInfo, packetLens] = s.packetRecv();
    if ~isempty(packetLens) && packetLens > 0
        sec   = s.record.data(1, end);
        msc   = s.record.data(2, end);
        count = s.record.data(3, end);
        tt    = s.getTime([1, 2], 0, 0.001);
        freq  = s.getFreq([1, 2], 256, 0.001);
        fprintf('[%02i:%02i:%02i][%4.0fHz] [%05i]\n', tt(1), tt(2), fix(tt(3) / 10), freq, count);

        osc.updateOscilloscope(s);
    end
end

s.close();
