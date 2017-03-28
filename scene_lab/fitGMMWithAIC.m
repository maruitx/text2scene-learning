function [mu, sigma, numK] = fitGMMWithAIC(X, maxK, alignMats)

if nargin == 3
    % enrich input data by jittering
    [instDim instNum] = size(X);
    
    inchToM = 0.0254;
    mToInch = 1/0.0254;
    stdVal = 0.05; % 5cm
    jitterMean = [0,0,0];
    jitterSigma = stdVal*mToInch*[1, 0, 0; 0, 1, 0; 0, 0, 1];
    jitterNum = 5;
    
    Xrich = zeros(instDim, jitterNum*instNum);
    for i=1:instNum
        aMatVec = alignMats(:,i);
        aMat = reshape(aMatVec,4,4)
        
        Xs = mvnrnd(jitterMean,jitterSigma, jitterNum)';
        Xs = [Xs; ones(1, jitterNum)];
        Xs = aMat*Xs
        
        Xs = Xs(1:3, :);
        X_col = repmat(X(:, i), 1, jitterNum)
        
        Xrich(:,(i-1)*jitterNum+1:i*jitterNum) = X_col + Xs;
    end
   
    Xrich = Xrich';
    save('enrich', 'Xrich', 'X');
    
    figure
    hold on
    scatter(Xrich(:,1),Xrich(:,2), 'filled');
    hold off
end

X = X';

figure
hold on
scatter(X(:,1),X(:,2), 'filled');
hold off

AIC = zeros(1,maxK);
GMModels = cell(1,maxK);
options = statset('MaxIter',500);

for k = 1:maxK
    GMModels{k} = fitgmdist(X,k,'Options',options, 'RegularizationValue',0.1);
    AIC(k)= GMModels{k}.AIC;
end

[minAIC,numK] = min(AIC);

mu = GMModels{numK}.mu;
sigma = GMModels{numK}.Sigma;


end
