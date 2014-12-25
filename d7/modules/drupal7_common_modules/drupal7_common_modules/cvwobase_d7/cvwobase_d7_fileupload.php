<?php
// Copyright (c) 2012-2013
//               Computing for Volunteer Welfare Organizations (CVWO)
//               National University of Singapore
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
// File Upload class
//
class CVWOFileUpload
{
    // location to save files
    private $uploaddir;
	
	private $last_uploaded_id;

    function CVWOFileUpload()
    {
    	$this->last_uploaded_id = 0;
    }

	/**
	 * Retrieve ID of last successfully uploaded file
	 * @return 
	 * 		unique file id, nonzero if valid
	 */
	function getLastUploadedFileID()
	{
		return $this->last_uploaded_id;
	}
	

    /**
     * Save uploaded file. Call this from the form submit handler
     * @return
     * 		true if success, false if not
     * @param object $file_field[optional]
     * 		field name of the form
     * @param object $file_type[optional]
     * 		acceptable MIME type substring, e.g. 'image/'
     */
    function saveFile($file_field = 'attachment', $file_type = '')
    {
        if ( isset ($_FILES[$file_field]) && ! empty($_FILES[$file_field]['name']))
        {

        	// we only care about single file uploads here
			$uploadkey = '';

			//the original name of the file is $_FILES[$file_field]['name'][$upload_key]

			if(is_array($_FILES[$file_field]['name']))
			{
				foreach($_FILES[$file_field]['name'] as $key => $val)
				{
					$uploadkey = $key;
					break;
				}
			}
      			
            // valid attachment ?
            if ($file_type != '')
            {
                if (!eregi($file_type, (($uploadkey == '') ?
					$_FILES[$file_field]['type'] :
					$_FILES[$file_field]['type'][$uploadkey])))
                {
                    // invalid data mime type found
                    return false;
                }
            }


            //01. we store the file into the file_managed table temporarily, and get the file object
            $file = file_save_upload($uploadkey,array(),NULL);

            //02. we get the file id in the file_managed table, and by using the file id together with the original filename, we create the new filename
            //also, we store the file id into the last_upload_id
            $filename= $file->fid.$_FILES[$file_field]['name'][$uploadkey];
    		$this->last_uploaded_id =$file->fid;

            //03. we save the file to be uploaded into the public file directory.
            $file = file_move($file, 'public://'.$filename, FILE_EXISTS_REPLACE);
			
            //04. set file the status to be permanently and save the file.
			$file->status = FILE_STATUS_PERMANENT;

			variable_set('file_id',$file->fid);
			file_save($file);

			//upload success;
			return true;
        }
        else{
            // invalid attachment data found
            return false;
        }
    }
	

	/**
	 * Return directly-accessible path of uploaded file
	 * @return 
	 * 		path string
	 * @param object $id
	 */
	static
	//API file function
	function getFilePath($file_id=NULL){
		$result=cvwo_select('file_managed','f')
				->fields('f',array('uri'))
				->condition('f.fid',$file_id,'=')
				->execute();
		if($uri=$result->fetchField()){
			// return  drupal_get_path("module" ,LIONS_CARD_MODULE_NAME) . '/' . 'pics'.'/'.$stor_name;
			// $publicFilePath=variable_get('file_' . file_default_scheme() . '_path', conf_path() . '/files');
			// $filename=$id.$filename;
			file_create_url($uri);
			$img_path =file_create_url($uri);
			return $img_path;
		}

	return NULL;


	}
    
    /**
     * Return the width and height of the scaled image.
     * @return 
     *      An array containing the width and height of the scaled image.
     * @param object $id
     *      The picture id of the photo.
     * @param int $max_width
     *      The maximum width you want the image to have when it is displayed.
     * @param int $max_height [OPTIONAL]
     *      The maximum height you want the image to have when it is displayed.
     */

    static
    function rescaleImage($id,$max_width,$max_height=NULL){
		if(is_null($max_height)){
       		$max_height = $max_width;
		}

		//$result = cvwo_query('SELECT filename FROM {'.'file_managed'.'} WHERE fid = :id', array(':id'=>$id));
		$result = file_load($id);
		if($result){
			// $publicFilePath=variable_get('file_' . file_default_scheme() . '_path', conf_path() . '/files');
			// $filename=$id.$res['filename'];
			$img_path = file_create_url($result->uri);
			$img_size = getimagesize($result->uri);

			$img_width = $img_size[0];
			$img_height = $img_size[1];
            
			$scale = min($max_width / $img_width, $max_height / $img_height, 1); //The smallest possible value is 1.
			$scale_width = round($scale * $img_width);
			$scale_height = round($scale * $img_height);
            
			return array($scale_width, $scale_height);
		}

		return array(0, 0);
    
	}

}
