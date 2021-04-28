This respority is mainly borrow and copy code from https://github.com/tensorflow/runtime.
original LICENSE is in ORI_LICENSE.

For easy of use, this repo remove some complicated designed code and remove many third-party depend.
Now it's only based on abseil ,googletest, eigen  

Some custom execution mode such as GraphMode and TaskGraphMode are added.

Start to add some coroutine functions and tools. Currently coroutine are just under development and should not be used.

Please refer to documents for more detailed explantion.

Feature Plan:  
1. Add unified coroutine and function task framework to process jobs.  
2. Add more utility functions for easy use this library.  
