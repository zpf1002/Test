# -*- coding:utf-8 -*-


import urllib
import urllib2
import re
import requests
import tool
import os


def dowmloadPic(html, keyword):
    pic_url = re.findall('"objURL":"(.*?)",', html, re.S)
    i = 0
    print '找到关键词:' + keyword + '的图片，现在开始下载图片...'
    for each in pic_url:
        print '正在下载第' + str(i + 1) + '张图片，图片地址:' + str(each)
        try:
            pic = requests.get(each, timeout=10)
        except requests.exceptions.ConnectionError:
            print '【错误】当前图片无法下载'
            continue
        string = 'pictures\\' + keyword + '_' + str(i) + '.jpg'
        fp = open(string.decode('utf-8').encode('cp936'), 'wb')
        fp.write(pic.content)
        fp.close()
        i += 1

    # main():
    # result = requests.get(url)
    # dowmloadPic(result.text, word)
    # exit(-1)

if __name__ == '__main__':

    x = 0
    page_index = 0
    each_page = 0
    is_next = True
    word = raw_input("Input key word: ")

    while is_next:
        # url = 'http://image.baidu.com/search/flip?tn=baiduimage&ie=utf-8&word='+word+'&ct=201326592&v=flip'
        url = 'http://image.baidu.com/search/flip?tn=baiduimage&ie=utf-8&word='+word+'&pn='+str(page_index)+'&v=flip'

        request = urllib2.Request(url)
        try:
            response = urllib2.urlopen(request)
        except urllib2.HTTPError, e:
            print e.code
            exit(-1)
        except urllib2.URLError, e:
            print e.reason
            exit(-1)
        else:
            print 'URL OK'

        html = response.read()
        reg = '"objURL":"(.*?)",'      # 正则表达式，得到图片地址
        imglist = re.findall(reg, html, re.S)
        page_index += len(imglist)
        if each_page == 0:
            each_page = len(imglist)
        if each_page > len(imglist):
            is_next = False

        for imgurl in imglist:
            print '正在下载第' + str(x + 1) + '张图片，图片地址:' + str(imgurl)
            try:
                pic = requests.get(imgurl, allow_redirects=False, timeout=10)
            except requests.exceptions.ConnectionError:
                print '【错误】当前图片无法下载'
                continue
            except requests.exceptions.ReadTimeout:
                print '【错误】当前图片无法下载'
                continue
            except requests.exceptions.ChunkedEncodingError:
                print '【错误】当前图片无法下载'
                continue
            string = 'D:\\E\\' + word + '_' + str(x) + '.jpg'
            fp = open(string.decode('utf-8').encode('cp936'), 'wb')
            fp.write(pic.content)
            fp.close()
            x += 1
        print 'Have Completed %d jpg download !!!!' % page_index




