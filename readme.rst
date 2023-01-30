=================
x265 HEVC Encoder
=================

| **Read:** | Online `documentation <http://x265.readthedocs.org/en/master/>`_ | Developer `wiki <http://bitbucket.org/multicoreware/x265_git/wiki/>`_
| **Download:** | `releases <http://ftp.videolan.org/pub/videolan/x265/>`_ 
| **Interact:** | #x265 on freenode.irc.net | `x265-devel@videolan.org <http://mailman.videolan.org/listinfo/x265-devel>`_ | `Report an issue <https://bitbucket.org/multicoreware/x265/issues?status=new&status=open>`_

`x265 <https://www.videolan.org/developers/x265.html>`_ is an open
source HEVC encoder. See the developer wiki for instructions for
downloading and building the source.

x265 is free to use under the `GNU GPL <http://www.gnu.org/licenses/gpl-2.0.html>`_ 
and is also available under a commercial `license <http://x265.org>`_ 

## **NOTE（踩坑记录）**  
- 缺少nasm：
   `pacam -S  pacman -S mingw-w64-x86_64-nasm`
- 在windows中使用cmake时由于存在git的环境变量导致cmake报错"list GET given empty list";
  `去除git环境变量后,重启cmake，清除cache，重新configure，成功编译。`  
