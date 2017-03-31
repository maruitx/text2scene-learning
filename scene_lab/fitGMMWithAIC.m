function [isFitSuccess, mus, sigmas, weights, numComp] = fitGMMWithAIC(X, maxK, alignMats)

isFitSuccess = 0;

% % remove outlier
% Xf =  removeOutlier(X');
% X = Xf';

vis = 0;
if vis
    figure
    hold on
    scatter(X(1,:),X(2,:), 'filled');
    hold off
end

if nargin == 3
    % enrich input data by jittering
    [instDim instNum] = size(X);
    
    inchToM = 0.0254;
    mToInch = 1/0.0254;
    stdVal = 0.05; % 5cm
    jitterMean = [0,0,0];
    jitterSigma = stdVal*mToInch*[1, 0, 0; 0, 1, 0; 0, 0, 1];
    jitterNum = 20;
    
    Xrich = zeros(instDim, jitterNum*instNum);
    for i=1:instNum
        aMatVec = alignMats(:,i);
        aMat = reshape(aMatVec,4,4);
        aMat(:,4) = [0;0;0;1];  % ignore the translation
        
        jitPos = mvnrnd(jitterMean,jitterSigma, jitterNum)';
        jitPos = [jitPos; ones(1, jitterNum)];
        jitPos = aMat*jitPos;
        jitPos = jitPos(1:3,:);
        
        jitDir = normrnd(0, sqrt(5*pi/180), 1, jitterNum);
        
        Xs = [jitPos; jitDir];
        X_col = repmat(X(:, i), 1, jitterNum);
        
        Xrich(:,(i-1)*jitterNum+1:i*jitterNum) = X_col + Xs;
    end
    
    X = Xrich';
   
    if vis
        figure
        hold on
        scatter(X(:,1),X(:,2), 'filled');
        hold off
    end
else
    X = X';
end

AIC = zeros(1,maxK);
GMModels = cell(1,maxK);
options = statset('MaxIter',500);

for k = 1:maxK
    try
        GMModels{k} = fitgmdist(X,k,'Options',options, 'RegularizationValue',0.1);
        AIC(k)= GMModels{k}.AIC;
        isFitSuccess = 1;
    catch
        figure
        hold on
        scatter(Xrich(:,1),Xrich(:,2), 'filled');
        hold off
        save('GMM_failure', 'X')
    end
end

[minAIC,numComp] = min(AIC);

if ~isempty(GMModels{numComp})
    mus = GMModels{numComp}.mu;
    sigmas = GMModels{numComp}.Sigma;
    weights = GMModels{numComp}.ComponentProportion;
else
    mus = [];
    sigmas = [];
    weights = [];
end

%save('GMM', 'mus', 'sigmas', 'weights')
end

function X =  removeOutlier(X)
% each row of X is a data point
o = moutlier1(X,0.1);
X(o, :) = [];
end
