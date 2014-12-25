<?php
// Copyright (c) 2007-2008
//			   Computing for Volunteer Welfare Organizations (CVWO)
//			   National University of Singapore
//
// Permission is hereby granted, free of charge, to any person obtainin
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject
// to the following conditions:
//
//
// 1. The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// 2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Email process
//
//require_once('cvwobase_queue.php');
require_once('cvwobase_email.php');
//require_once('cvwobase_')

// check for command line arguments
if ($argc != 2) {
	die('Invalid arguments');
}

// name of email queue to process
$queuename = $argv[1];

$queue = new CVWOQueue($queuename);

// iterate between email batches
while($data = $queue->pop())
{
	$email = new CVWOEmail();
	$email->setHeaders($data['data']['extraHeaders']);
	$email->setContent($data['data']['content']);

	$email->sendImmediate($data['data']['to'],false,$data['data']['cc'],$data['data']['bcc'], $data['data']['attachment'], $data['data']['by_uid']);
}

// completed processing queue
