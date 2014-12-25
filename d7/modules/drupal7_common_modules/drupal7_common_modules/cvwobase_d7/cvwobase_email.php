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
// Emailing helper class
//


@require_once "Mail.php";
@require_once "Mail/mime.php";
/* End of Change */
require_once drupal_get_path("module", CVWOBASE_MODULE)."/cvwobase_email.php";
require_once drupal_get_path("module", CVWOBASE_MODULE)."/cvwobase_d7_api.php";

class CVWOEmail
{
	// server settings
	// private $mailname = 'nuscvwo';
	// private $mailpass = 'vwo2007vwo2007';
	// private $mailhost = 'ssl://smtp.gmail.com';
	// private $mailport = 465;
	// private $timeout = 20;


	// YMCA Mail Serverr
	// private $mailname = 'vmstest';
	// private $mailpass = 'password@vms';
	// private $mailhost = 'mail.ymca.org.sg';
	// private $mailport = 25;
	// private $timeout = 20;


	private $mailname = 'vms';
	private $mailpass = 'password@vms';
	private $mailhost = 'webmail.ymca.org.sg';
	private $mailport = 25;
	private $timeout = 20;


	// used for mime: generating all the contents and headers
	private $message;
	private $extraHeaders = array();
	private $htmlContent = "";

	private $queue_r = null;

	/**
	 * Default Ctor
	 * @return void
	 * @param string from (optional)
	 * 		Address it is sent from
	 * @param string subject (optional)
	 * 		Mail subject
	 */
	function CVWOEmail($from="",$subject=""){
		$params = array();
//		$params['eol'] = "\n";
		$params['head_encoding'] = "8bit";
		$params['text_encoding'] = "8bit";
		$params['html_encoding'] = "8bit";
		$params['head_charset'] = "UTF-8";
		$params['text_charset'] = "UTF-8";
		$params['html_charset'] = "UTF-8";

		$this->message = new Mail_mime($params);
		$this->extraHeaders['From'] = $from;
		$this->extraHeaders['Reply-To'] = $from;
		$this->extraHeaders['Subject'] = $subject;
	}

	// subject
	function setSubject($subject){
		$this->extraHeaders['Subject'] = $subject;
	}
	function getSubject(){
		return $this->extraHeaders['Subject'];
	}

	// content
	// assumption: the content is in HTML
	function setContent($contents){
		$this->htmlContent = $contents;
		$this->message->setHTMLBody($contents);
	}
	function getContent(){
		return $this->htmlContent;
	}

	// from address
	function setFromAddress($from_addr){
		$this->extraHeaders['From'] = $from;
	}
	function getFromAddress(){
		return $this->extraHeaders['From'];
	}

	// reply-to
	function setReplyToAddress($reply_addr){
		$this->extraHeaders['Reply-To'] = $reply_addr;
	}
	function getReplyToAddress(){
		return $this->extraHeaders['Reply-To'];
	}

	
	// manipulating headers 
	function addHeader($key,$val){
		$this->extraHeaders[$key] = $val;
	}
	function setHeaders($headers){
		$this->extraHeaders = $headers;
	}
	function getExtraHeaders(){
		return $this->extraHeaders;
	}

	/**
	 * Replacement function often used for mailmerge
	 * 
	 * @return
	 * 		new CVWOEmail with replaced content
	 * @param object $conditions
	 * 		an array of things to replace, $symbol-to-be-replaced => $by-this-value
	 */
	function replaceContent(array $conditions)
	{
		$resultant = $this->htmlContent;
		// lets replace $key with $val in order as they are retrieved
		foreach ($conditions as $key => $val)
		{
			$resultant = str_replace($key, $value, $resultant);
		}
		
		// deep copy the current instance first
		$newMail = unserialize(serialize($this));
		// then replace the old content with this new one
		$newMail->setContent($resultant);
		return $newMail;
	}
	
	//
	/** Send out the email immediately
	 *  
	 *  @param string $recipients - 
	 *		if doesn't sepcify the $to/$cc address, the recipients would be treated as bcc
	 *	@param bool $printoutput - 
	 *	@param string $to - 
	 *		set the to address shown by the mail server
	 *	@param string $cc -
	 *		the cc address
	 *	@param array attachment - 
	 *		the attachment array can be in either of the 2 forms:
	 *			a) each node of the array is stored as fileName=>fileData;
	 *			b) each node of the array is an filePath. 
	 */
	function sendImmediate($to, $printoutput = false, $cc = "", $bcc = "", $attachment=array(),$uid = 0){
		
		// support $to to be array 
		if(is_Array($to)){
			$to = implode(",",$to);
		}
		if(is_Array($cc)){
			$cc = implode(",",$cc);
		}
		if(is_Array($bcc)){
			$bcc = implode(",",$bcc);
		}
		
		// set recipients
		if($to != ""){
			$recipients = $to.(($cc=="")?"":(", ".$cc)).(($bcc=="")?"":(", ".$bcc));
		} else if($cc != ""){
			$recipients = $cc.(($bcc=="")?"":(", ".$bcc));
		} else{
			$recipients = $bcc;
		}

		// add in attachment
		if(isset($attachment['fileName'])){
			foreach($attachment['fileName'] as $key=>$value){
				$this->message->addAttachment(
					$attachment['fileContent'][$key],
					null,
					//$attachment['fileType'][$key],	// file type
					$value,	// file name
					false, // is file name or file content 
					"base64",	// encoding of the file content; base64 is the default
					'attachment',	// disposition  
					"",	// char set
					'',	// language
					'',	// location
					"",	// n_encoding
					"base64",	// f_encoding
					'',	// description
					"UTF-8"	// h_charset
				);
			}
		} else{
			foreach($attachment as $key=>$value){
				$this->message->addAttachment($value,
					'application/octet-stream',	// file type
					"",	// file name
					true, // is file name or file content 
					"base64",	// encoding of the file content; base64 is the default
					'attachment',	// disposition  
					"",	// char set
					'',	// language
					'',	// location
					"",	// n_encoding
					"base64",	// f_encoding
					'',	// description
					"UTF-8"	// h_charset

				);
			}
		}

		if($to != "") $this->addHeader("To",$to); // add in to
		if($cc != "") $this->addHeader("Cc",$cc); // add in cc

		// create the mail factory
		// assumption: using SMTP
		$mail = Mail::factory('smtp',
			array (
			'host' => $this->mailhost,
			'port' => $this->mailport,
			'auth' => true,
			'username' => $this->mailname,
			'password' => $this->mailpass,
			'timeout' => $this->timeout,
		));

		// send out the email, and return the sending result 
		$body = $this->message->get();
		
		
		$header = $this->message->headers($this->extraHeaders);
		

		$res = $mail->send($recipients,$header,$body);
		
		if(PEAR::isError($res)){
			echo $res;
			//*****$failedQueue = new CVWOQueue("failed_mail_list");
			$failedQueue = DrupalQueue::get('failedQueue',TRUE);
			//
			$data = array(
				'content'=>$this->getContent(),
				'extraHeaders' => $this->getExtraHeaders(),
				'to' => $to,			// need
				'cc' => $cc,			// need
				'bcc' => $bcc,			// need
				'attachment' => $attachment,	// info about the file
				'by_uid' => $uid,		// need
				'err_info' => $res,		// need
			);
			//*****$failedQueue->push($data);
			$failedQueue->createItem($data);
			//
			// store information about the failed mails

			//***$errMSG = new CVWOQueue("failed_mail_info");
			$errMSG = DrupalQueue::get('errMSG',TRUE);
			//
			$err_info = array(
				'subject' => $this->getSubject(),
				'to' => $to,			// need
				'cc' => $cc,			// need
				'bcc' => $bcc,			// need
				'hasAttachment' => empty($attachment),	// whether has attachment or not 
				'by_uid' => $uid,		// need
				'err_info' => $res,		// need
			);
			//****$errMSG->push($err_info);
			$errMSG->createItem($err_info);
			//

			return false;
		} else{
			return true;
		}
	}

	// the sendQueued function; not recommended using
	// assumption: all sent as bcc
	function sendQueued($to,$queue = null,$cc = "", $bcc = "", $attachment=array(),$uid=0){
		if($queue==null){
			if($this->queue_r == null){
				// generate new queue id if required
				//***$this->queue_r = new CVWOQueue('mail_'.rand(0,999).'_'.rand(0,999).'_'.time());
				$this->queue_r = DrupalQueue::get('mail_'.rand(0,999).'_'.rand(0,999).'_'.time(),TRUE);
				//
			}
		} else{
			$this->queue_r = $queue;
		}

		$mail_temp = array(
			'content' => $this->getContent(),
			'extraHeaders' => $this->getExtraHeaders(),
			'to' => $to,
			'cc' => $cc,
			'bcc' => $bcc,
			'attachment' => $attachment,
   			'by_uid' => $uid,
		);

		//***$this->queue_r->push($mail_temp);
		$this->queue_r->createItem($mail_temp);
		//
		return $this->queue_r;
	}
	function sendQueuedFinalize(){
		if($this->queue_r == null){
			return false;
		}

		//***$execstring = 'php '.dirname(__FILE__).'/cvwobase_email_process.php '.$this->queue_r->getName();
		$execstring = 'php '.dirname(__FILE__).'/cvwobase_email_process.php '.$this->queue_r->name;
		cvwobase_exec($execstring);
		return true;
	}

}
