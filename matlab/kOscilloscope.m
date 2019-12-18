% 
%       __            ____
%      / /__ _  __   / __/                      __  
%     / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
%    / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
%   /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
%                     /_/   github.com/KitSprout    
%  
%  @file    kOscilloscope.m
%  @author  KitSprout
%  @date    Dec-2019
%  @brief   
% 

classdef kOscilloscope < handle

properties (SetAccess = public)

end

properties (SetAccess = private)
	ax;
    available = false;
    curve = struct;
    window = struct;
    count = 0;
end

methods

    % osc = kOscilloscope()
    % osc = kOscilloscope(ax)
    function osc = kOscilloscope( varargin )
        % input setting
        switch nargin
            case 0
                fig = figure('Position', [100, 200, 1200, 460], 'color', 'w');
            	tiledlayout(fig, 1, 1);
                ax = nexttile;
            case 1
                ax = varargin{1};
        end
        osc.ax = ax;

        % default window size
        osc.setWindowSize((2^15)*[1,-1], 800);

        % default curve setting
        osc.curve.obj = [];
        osc.curve.buffersize = 4*1024;
        osc.curve.data = [];
        osc.curve.num = 0;
        osc.curve.select = 0;
        osc.curve.selectnum = 0;
        osc.curve.color = cell(0);
        osc.curve.scale = [];
        osc.curve.offset = [];
        osc.curve.markersize = [];
        osc.curve.info = {
            '#D95319', 1, 0, 12;
            '#77AC30', 1, 0, 12;
            '#0072BD', 1, 0, 12;
            '#7E2F8E', 1, 0, 12;
            '#EDB120', 1, 0, 12;
            '#4DBEEE', 1, 0, 12;
            '#A2142F', 1, 0, 12;
        };
    end

    % setWindowSize([ymax, ymin], xwidth)
	function setWindowSize( osc, range, width )
        osc.window.width = width;
        osc.window.displayPos = osc.window.width * (-0.9);
        osc.window.xmax = osc.window.displayPos + osc.window.width;
        osc.window.xmin = osc.window.displayPos;
        osc.window.ymax = range(1);
        osc.window.ymin = range(2);
    end

    % setCurveBufferSize(buffersize)
	function setCurveBufferSize( osc, buffersize )
        osc.curve.buffersize = buffersize;
    end

    % setCurveInfo()
    % setCurveInfo(info)
	function setCurveInfo( osc, varargin )
        if nargin == 2
            info = varargin{1};
            osc.curve.info = info;
        end
        osc.curve.num = size(osc.curve.info, 1);
        osc.curve.color = cell(1, osc.curve.num);
        osc.curve.scale = zeros(1, osc.curve.num);
        osc.curve.offset = zeros(1, osc.curve.num);
        osc.curve.markersize = zeros(1, osc.curve.num);
        for i = 1 : osc.curve.num
            osc.curve.color{i} = osc.curve.info{i, 1};
            osc.curve.scale(i) = osc.curve.info{i, 2};
            osc.curve.offset(i) = osc.curve.info{i, 3};
            osc.curve.markersize(i) = osc.curve.info{i, 4};
        end
    end

    % TODO: setCurveColor
	function setCurveColor( osc, index, color )
        for i = 1 : size(color, 2)
            osc.curve.color{index(i)} = color{i};
        end
    end

    % TODO: setCurveScale
	function setCurveScale( osc, index, scale )
        osc.curve.scale(index) = scale;
    end

    % TODO: setCurveOffset
	function setCurveOffset( osc, index, offset )
        osc.curve.offset(index) = offset;
    end

    % TODO: setCurveMakersize
	function setCurveMakersize( osc, index, markersize )
        osc.curve.markersize(index) = markersize;
    end

    % initOscilloscope(index, xlabel, ylabel)
	function initOscilloscope( osc, index, xname, yname )
        % figure init
        hold(osc.ax, 'on');
        grid(osc.ax, 'on');

        xlabel(osc.ax, xname);
        ylabel(osc.ax, yname);
        axis(osc.ax, [osc.window.xmin, osc.window.xmax, osc.window.ymin, osc.window.ymax]);
        osc.ax.YTickLabel = num2str(osc.ax.YTick','%d');

        osc.curve.select = index;
        osc.curve.selectnum = size(osc.curve.select, 2);
        if osc.curve.selectnum > osc.curve.num
            error('select number > max curve number!!!');
        end
        for i = 1 : osc.curve.selectnum
            if ischar(osc.curve.color{i}) && size(osc.curve.color{i}, 2) ~= 7
                osc.curve.obj(i) = plot(osc.ax, 0, 0, osc.curve.color{i});
            else
                plt = plot(osc.ax, 0, 0);
                plt.Color = osc.curve.color{i};
                osc.curve.obj(i) = plt;
            end
        end
        if isempty(osc.curve.scale)
            osc.curve.scale = ones(1, osc.curve.num);
        end
        if isempty(osc.curve.offset)
            osc.curve.offset = zeros(1, osc.curve.num);
        end
        if isempty(osc.curve.markersize)
            osc.curve.markersize = 12 * ones(1, osc.curve.num);
        end
        osc.curve.data = zeros(osc.curve.num, osc.curve.buffersize);

        osc.available = ishandle(osc.ax);
    end

    % updateOscilloscope(data)
    function updateOscilloscope( osc, data )
        delete(osc.curve.obj);

        % TODO: y-axis auto-adjust

        lens = size(data, 2);
        osc.count = osc.count + lens;
        if size(data, 1) > size(osc.curve.data, 1)
            % expand buffer
            osc.curve.data((end + 1) : size(data, 1), :) = 0;
        end
        osc.curve.data = [osc.curve.data(:, (lens + 1) : end), data];
        runtimes = (osc.count - osc.window.width + 1) : osc.count;

        for i = 1 : osc.curve.selectnum
            idx = osc.curve.select(i);
            if osc.count > osc.window.width - 1
                cvdata = osc.curve.data(idx, end - osc.window.width + 1 : end);
            else
                cvdata = [zeros(1, osc.window.width - osc.count), osc.curve.data(idx, end - osc.count + 1 : end)];
            end
            cvdata = cvdata .* osc.curve.scale(i) + osc.curve.offset(i);

            if ischar(osc.curve.color{i}) && size(osc.curve.color{i}, 2) ~= 7
                plt = plot(osc.ax, runtimes, cvdata, osc.curve.color{i});
            else
                plt = plot(osc.ax, runtimes, cvdata);
                plt.Color = osc.curve.color{i};
            end
            plt.MarkerSize = osc.curve.markersize(i);
            osc.curve.obj(i) = plt;
        end

        osc.window.xmin = osc.window.xmin + lens;
        osc.window.xmax = osc.window.xmin + osc.window.width;
        axis(osc.ax, [osc.window.xmin, osc.window.xmax, osc.window.ymin, osc.window.ymax]);
        drawnow

        osc.available = ishandle(osc.ax);
    end

end

end
