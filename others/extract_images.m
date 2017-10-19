load('../data/scene_classes.mat')

imgFolder = '../data/images/';
extractedFolder = '../data/extracted/';
extractedImgFolder = [extractedFolder 'images/'];

if ~exist(extractedFolder, 'dir')
   mkdir(extractedFolder) 
end

if ~exist(extractedImgFolder, 'dir')
   mkdir(extractedImgFolder) 
end

for i=1:length(class_labels)
    if      class_labels(i) == 2 ||...    % office
            class_labels(i) == 4 ||...    % living_room
            class_labels(i) == 5 ||...    % bedroom
            class_labels(i) == 8 ||...    % home_office
            class_labels(i) == 11 ||...   % study
            class_labels(i) == 12         % dining_room
        
        % copy images
        imgId = num2str(i, '%04d');
        sourceImage = [imgFolder imgId '.jpg'];
        extractImage = [extractedImgFolder imgId '.jpg'];
        copyfile(sourceImage, extractImage)
    end
end
