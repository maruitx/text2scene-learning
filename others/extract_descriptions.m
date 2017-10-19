load('../data/scene_classes.mat')

descriptionInfoFolder = '../data/descriptions_info/';
imgFolder = '../data/images/';
extractedFolder = '../data/extracted/';
extractedImgFolder = [extractedFolder 'images/'];

if ~exist(extractedFolder, 'dir')
   mkdir(extractedFolder) 
end

if ~exist(extractedImgFolder, 'dir')
   mkdir(extractedImgFolder) 
end

textFile = fopen([extractedFolder 'text_descriptions.txt'], 'w');

for i=1:length(class_labels)
    if      class_labels(i) == 2 ||...    % office
            class_labels(i) == 4 ||...    % living_room
            class_labels(i) == 5 ||...    % bedroom
            class_labels(i) == 8 ||...    % home_office
            class_labels(i) == 11 ||...   % study
            class_labels(i) == 12         % dining_room
        
       % save descriptions
        imgId = num2str(i, '%04d');
        load([descriptionInfoFolder 'in' imgId '.mat'])
        fprintf(textFile, '%s\n', imgId);
        fprintf(textFile, '%s\n', descriptions.text);        
    end
end


fclose(textFile);