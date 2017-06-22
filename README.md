# Alibaba Cloud OSS Media C SDK 

[![Software License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](LICENSE)
[![GitHub Version](https://badge.fury.io/gh/aliyun%2Faliyun-media-c-sdk.svg)](https://badge.fury.io/gh/aliyun%2Faliyun-media-c-sdk)


## [README of Chinese](https://github.com/aliyun/aliyun-media-c-sdk/blob/master/README-CN.md)

## About
In many cases, we need to store the video captured by the camera to the [OSS](https://www.aliyun.com/product/oss). But we also have some considerations:

- The `AccessKeyID` and `AccessKeySecret` cannot be stored permanently on the device because it may be leaked.
- The device only allows file uploads and downloads and does not allow administrative permissions such as deletion and configuration modification.
- Webpages can be provided for you to manage your own videos.
- Precise control over the device permission is provided.
- The permission for the device has a validity period. A permission for a device cannot be held permanently.
- The audio and video files output by the camera are expected to be directly watched through the *HLS* protocol.

Out of the above considerations, we launched the *OSS MEDIA C SDK* built based on the *OSS C SDK*, which can conveniently solve the above mentioned issues and provide more improved and easy-to-use solutions for the audio and video industry.

## Version
 - Current version: V2.0.2. 

### Compatibility
- Because the underlying C SDK is not compatible with previous versions, the Media C SDK is not compatible with 1.x.x. Incompatible interfaces mainly involve those related with *list* operations. 

## Dependency
 - [OSS C SDK 3.4.0](https://github.com/aliyun/aliyun-oss-c-sdk)

## Usage
 - Example project: [Refer to sample section](sample/).
 - Documentation: [OSS C MEDIA SDK Documentation](https://help.aliyun.com/document_detail/oss/sdk/media-c-sdk/preface.html). 

## License

- MIT

## Contact us
- [Alibaba Cloud OSS official website](http://oss.aliyun.com).
- [Alibaba Cloud OSS official forum](http://bbs.aliyun.com). 
- [Alibaba Cloud OSS official documentation center](http://www.aliyun.com/product/oss#Docs).
- Alibaba Cloud official technical support: [Submit a ticket](https://workorder.console.aliyun.com/#/ticket/createIndex).
