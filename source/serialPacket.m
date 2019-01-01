
clc, clear, close all;

s = kSerial(115200, 'clear');
s.setRecordBufferSize(1024 * 64);
s.open();

sv = struct;
sv.t_unit = 0.001;
% sv.g_sens = 16.4;   % dps
% sv.a_sens = 16384;  % g
% sv.m_sens = 6.6;    % uT
sv.t_idx = 1 : 2;
% sv.g_idx = sv.t_idx(end) + (1 : 3);
% sv.a_idx = sv.g_idx(end) + (1 : 3);
% sv.m_idx = sv.a_idx(end) + (1 : 3);

% {
while s.ks.lens < 2000
    [packetData, packetInfo, packetLens] = s.packetRecv();
    if ~isempty(packetLens) && packetLens > 0
%         [freq, tt] = s.getFreqAndTime(0.4);
        [freq, tt] = s.getFreqAndTime(sv.t_idx, sv.t_unit, 0.4);

        fprintf('[%06d]', s.ks.lens);
        fprintf('[%03d]', packetLens);
        fprintf('[%02i:%02i:%02i]', tt(2), tt(3), fix(tt(4) / 10));
        fprintf('[%4dHz] ', freq);
        fprintf('%s, %d bytes, %02X %02X %02X', s.typeConv(s.ks.info(2, end)), s.ks.info(1, end), s.ks.info(3:end, end));

        fprintf('\n');
    end
end
%}
s.close();

% {
% check packet
% [rate, lost, dc] = s.getLostRate();
[rate, lost, dc] = s.getLostRate(sv.t_idx, sv.t_unit, freq);
if lost == 0
    fprintf('---- [%05.2f%%] No packet loss ( %i / %i ) ----\n', rate * 100, lost, s.ks.lens);
else
    fprintf('---- [%05.2f%%] Packet loss ( %i / %i ) ----\n', rate * 100, lost, s.ks.lens);
end
% plot(1 : s.ks.lens - 1, dc)
%}
%{
filename = s.save2mat('log/rawData', sv);
%}
