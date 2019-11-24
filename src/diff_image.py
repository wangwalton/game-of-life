import cv2

img1 = cv2.imread('temp1.jpg')
img2 = cv2.imread('temp2.jpg')

cv2.imwrite('diff.jpg', img2-img1)
