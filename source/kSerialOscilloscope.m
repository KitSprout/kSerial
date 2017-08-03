% 
%       __            ____
%      / /__ _  __   / __/                      __  
%     / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
%    / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
%   /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
%                     /_/   github.com/KitSprout    
%  
%  @file    kSerialOscilloscope.m
%  @author  KitSprout
%  @date    02-Jul-2017
%  @brief   
% 

classdef kSerialOscilloscope < handle

properties (SetAccess = public)
	curveChannel;
    curveScale;
	curveColor;
    curveOffset;
end

properties (SetAccess = private)
    fig;
    curve;
    window = struct;
end

methods

    function osc = kSerialOscilloscope( ~ )
        osc.window.width = 800;
        osc.window.displayPos = osc.window.width * (-0.9);
        osc.window.xmax = osc.window.displayPos + osc.window.width;
        osc.window.xmin = osc.window.displayPos;
        osc.window.ymax =  200;
        osc.window.ymin = -200;
    end

	function setWindow( osc, range, width )
        osc.window.width = width;
        osc.window.displayPos = osc.window.width * (-0.9);
        osc.window.xmax = osc.window.displayPos + osc.window.width;
        osc.window.xmin = osc.window.displayPos;
        osc.window.ymax = range(1);
        osc.window.ymin = range(2);
    end

	function initOscilloscope( osc, fig, xName, yName )
        osc.fig = fig;
        grid(osc.fig, 'on');
        hold(osc.fig, 'on');
        xlabel(osc.fig, xName);
        ylabel(osc.fig, yName);
        axis(osc.fig, [osc.window.xmin, osc.window.xmax, osc.window.ymin, osc.window.ymax]);
        for i = 1 : size(osc.curveChannel, 2)
            osc.curve(i) = plot(osc.fig, 0, 0, osc.curveColor(i));
        end
        if isempty(osc.curveScale)
            osc.curveScale = ones(size(osc.curveChannel));
        end
        if isempty(osc.curveOffset)
            osc.curveOffset = zeros(size(osc.curveChannel));
        end
    end

    function updateOscilloscope( osc, s )
        delete(osc.curve);
        runtimes = (s.record.count - osc.window.width + 1) : s.record.count;
        for i = 1 : size(osc.curveChannel, 2)
            osc.curve(i) = plot(osc.fig, runtimes, s.record.data(osc.curveChannel(i), end - osc.window.width + 1 : end) .* osc.curveScale(i) + osc.curveOffset(i), osc.curveColor(i));
        end
        osc.window.xmin = osc.window.xmin + s.packet.availableCount;
        osc.window.xmax = osc.window.xmin + osc.window.width;
        axis(osc.fig, [osc.window.xmin, osc.window.xmax, osc.window.ymin, osc.window.ymax]);
        drawnow
    end

end

end
