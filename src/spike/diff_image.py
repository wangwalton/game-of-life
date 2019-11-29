import cv2
import os

img1 = cv2.imread('build/temp1.jpg')
img2 = cv2.imread('build/temp2.jpg')

cv2.imwrite('build/diff.jpg', img2-img1)
