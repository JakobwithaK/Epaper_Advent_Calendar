import os
from PIL import Image
import PIL.ImageOps    
import re
import copy
import struct
import shutil


#########################################

# Script will overwrite any existing file it is run multiple times
# user can specify path for searching and saving files
# user can add black borders to every image (it is good for menu icons etc)

# in case of error with PIL user must install this
# 'pip install Pillow' OR 'python -m pip install Pillow' OR 'python3 -m pip install Pillow'

# TIP for creating custom size images use website 'https://www.reduceimages.com/'

######################################xx

# array used for converting 8bit image depth to 4bit=16values for eInk in M5 Paper
GRAYSCALE_MAP = [0 for i in range(256)]

# option to let function create black edge around image (good for icons in menu)
# currently only single pixel is processed
createBlackEdge = 0

# path for searching
path = os.getcwd()

# debug output print
# 2 --> YES (ALL), 1 --> YES (BASIC), 0 --> NO
DEBUG_PRINT = 0

# create either single image = single file or single file = multiple images
# 1 --> single image=single file, 0 --> mutiple images=single file
SINGLE_IMAGE_FILE = 1

# flag to check for multiple file in single header writes
firstInFile = 1

def createGrayMap():


    GRAYSCALE_MAP_LOCAL = [0 for i in range(256)]

    x = -1
    for i in range(0, 256):
        if i % 16 == 0:
            x += 1
        GRAYSCALE_MAP_LOCAL[i] = 15 - x


    return GRAYSCALE_MAP_LOCAL


# function to search for all images with format .png, .bmp and .jpg in specified path and all of its subfolders
def searchForImages(search_path="C:/", verbose = 0):

    result_file_paths = []
    result_files = []

    # search for files in all folders in search path
    for root, dirs, files in os.walk(search_path):

        # go through every file =
        for f in files:
            x = os.path.join(root, f)

            if verbose==2:
                print("Current file path: " + x)
                print("Current file: " + f)

            # check if selected file is image with supported format
            if '.png' in x or '.bmp' in x or '.jpg' in x:
                if verbose:
                    print("Found image: " + f)

                # try to add found image to buffer for processing
                try:
                    #print((x, result.search('/(\w+)\.', x)[1].lower()))
                    #result.append((x, result.search('/(\w+)\.', x)[1].lower()))
                    if verbose:
                        print("Image: " + x + " added to buffer")
                    result_file_paths.append(x)
                    result_files.append(f)
                except:
                    print('ERROR addint image ' + x + " to buffer")

    ## return buffer with all found images
    return result_files, result_file_paths

# take image and convert it into array and then scale it to selected GRAYSCALE_MAP so its compatible with eInk in M5 Paper
def getBitmap(img, verbose = 0):

    if verbose==2:
        # show image before converting to basic gray scale
        img.show(title="Original Image")

    # output array buffer with converted image
    buffer = []
    byte = 0

    # converting image to default gray-scale 8bit
    imgGray = img.convert('L')

    if verbose==1:
        # show image after basic gray scale conversion
        imgGray.show(title="Original Gray Image")

    # create black edge around picture
    if createBlackEdge:
        for y in range(imgGray.height):
            imgGray.putpixel( (0, y ), 0)
            imgGray.putpixel( (imgGray.width-1, y), 0)

        for x in range(imgGray.width):
            imgGray.putpixel( (x, 0 ), 0)
            imgGray.putpixel( (x, imgGray.height-1), 0)

        if verbose==1:
            # show image after basic gray scale conversion and with added black edge
            imgGray.show(title="Gray Image with black borders")

    # go through all pixels in image and convert to predefined grayscale map
    for y in range(imgGray.height):
        for x in range(0, imgGray.width, 2):

            byte = GRAYSCALE_MAP[imgGray.getpixel((x, y))] << 4

            if x + 1 < imgGray.width:
                byte |= GRAYSCALE_MAP[imgGray.getpixel((x + 1, y))]



            # add processed pixel into array buffer
            buffer.append(byte)

    # converted image to selected grayscale map in array
    return bytes(buffer)

if __name__ == '__main__':

    print("\nSCRIPT: IMAGES TO GRAYSCALE IMAGE ARRAYS - M5 Paper compatible\n")

    # create gray scale map for 16 values = 4bit image depth
    GRAYSCALE_MAP = createGrayMap()

    # user can specify search folder for images and consequent converted files
    # example here
    # path = r'C:\Users\*userName*\Desktop\fileFolder'
    # UNCOMMENT AND SPECIFY
    #
    #path = r'C:\Users\Michal-Dell\Desktop\M5Paper_projects\M5EPD-main\tools\pictures'

    # search for all image files in specified path
    print("Seaching in path: " + path + "\n")
    file, files_path = searchForImages(search_path=path, verbose=DEBUG_PRINT)

    # print all found images
    print(str(len(file)) + " images found")
    for index in range(len(file)):
        print(" Found file: " + file[index])

        if DEBUG_PRINT:
            print("     Found image path: " + files_path[index])

    firstInFile = 1

    # convert every image into array with specified grascale map and save into current path with filename equal to processed file.txt
    for index in range(len(file)):

        image_array_buffer = []

        # open image
        orig_image = Image.open(files_path[index])
        image = orig_image.copy()

        #invert image
        image = PIL.ImageOps.invert(image)

        # process image into gray array image
        image_array_buffer = getBitmap(img=image, verbose=DEBUG_PRINT)

        # process file name so it wont containt .file_format
        txt_file_name = file[index]
        split_string = txt_file_name.partition('.')
        txt_file_name = split_string[0]

        #for item in split_string:
        #    print(item)

        # check if we found string/name of the file without file_format attached
        if "." in txt_file_name:
            print(" ERROR did not find string in filename")
            break

        if DEBUG_PRINT:
            print("New txt file name: " + txt_file_name)

        write_file_name = path + "/" + "generic_file.txt"

        ## either single image = single file or multiple images into single file AKA header file
        if SINGLE_IMAGE_FILE:

            write_file_name = path + "/" + txt_file_name + ".h"

            # save array to txt file
            f = open(write_file_name, "w+")
        else:
            write_file_name = path + "/ImageResource.h"
            # save array to txt file
            f = open(write_file_name, "a+")

            if firstInFile:
                # writing basic information into file to support it as header file
                f.write('#ifndef IMAGERESOURCE_H\n#define IMAGERESOURCE_H\n\n')
                f.write('// ----- ImageResource Overview -----\n')
                f.write('// Name, Width x Height\n')
                f.write('// Data to write\n')

                firstInFile = 0

        if DEBUG_PRINT==1:
            print("Writing to file: " + write_file_name)

        # writing image data to file
        f.write('const unsigned char ImageResource_%s_%dx%d[%d] = {\n\t' %(txt_file_name, image.width, image.height, len(image_array_buffer)))
        cnt = 0
        for byte in image_array_buffer:
            f.write('0x%02X, ' % byte)
            cnt += 1
            if cnt == 15:
                cnt = 0
                f.write('\n\t')

        f.write('};\n\n')

        # close right after writing data, file is no longer needed
        if SINGLE_IMAGE_FILE:
            f.close()

    # close write after all images are written
    if not SINGLE_IMAGE_FILE:
        f.write('\n\n#endif\n\n')

    if SINGLE_IMAGE_FILE:
        print("\nCreated single file for single image")
    else:
        print("\nCreated single file " + write_file_name + " for all images")

    print("\nALL IMAGES PROCESSED AND CREATED FILES ACCORDINGLY\n")


