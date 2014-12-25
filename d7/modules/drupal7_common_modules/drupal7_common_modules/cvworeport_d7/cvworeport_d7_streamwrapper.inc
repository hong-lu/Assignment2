<?php
/*
 * The MIT License
 *
 * Copyright 2011-2013 Computing for Volunteer Welfare Organizations (CVWO),
 * National University of Singapore.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

use \CVWO\Base\Database as db;

class CvwoReportStreamWrapper implements DrupalStreamWrapperInterface
{
	/**
	 * A generic resource handle. If this is a Resource, use fread/fwrite and friends;
	 * if this is a string, then read from it. If this is a resource it could also
	 * be used for directory enumeration, of which then use either readdir or
	 * the fetchRow (for database listings). If this is an array, then just use
	 * current, next and friends.
	 *
	 * @var mixed
	 */
	private $handle = null;

	/**
	 * The position of our handle. This is used iff $this->handle is a string and
	 * we have to give streamy access to the database code.
	 * @var integer
	 */
	private $handle_pos = null;

	/**
	 * Instance URI (stream).
	 *
	 * A stream is referenced as "scheme://target".
	 *
	 * @var String
	 */
	protected $uri;
	
	const SHARED_REPORTS_DIRECTORY = 'shared';

	/**
	 * Gets the path that the wrapper is responsible for.
	 * @TODO: Review this method name in D8 per http://drupal.org/node/701358
	 *
	 * @return
	 *   String specifying the path.
	 */
	public function getDirectoryPath()
	{
		return drupal_get_path('module', CVWOREPORT_MODULE) . '/' .
			self::SHARED_REPORTS_DIRECTORY;
	}

	/**
	 * Set the absolute stream resource URI.
	 *
	 * This allows you to set the URI. Generally is only called by the factory
	 * method.
	 *
	 * @param $uri
	 *   A string containing the URI that should be used for this instance.
	 */
	function setUri($uri)
	{
		$this->uri = $uri;
	}

	/**
	 * Returns the stream resource URI.
	 *
	 * @return
	 *   Returns the current URI of the instance.
	 */
	function getUri()
	{
		return $this->uri;
	}

	/**
	 * Returns a web accessible URL for the resource.
	 *
	 * This function should return a URL that can be embedded in a web page
	 * and accessed from a browser. For example, the external URL of
	 * "youtube://xIpLd0WQKCY" might be
	 * "http://www.youtube.com/watch?v=xIpLd0WQKCY".
	 *
	 * @return
	 *   Returns a string containing a web accessible URL for the resource.
	 */
	public function getExternalUrl()
	{
		//Not possible.
		assert(false);
		return null;
	}

	/**
	 * Returns the MIME type of the resource.
	 *
	 * @param $uri
	 *   The URI, path, or filename.
	 * @param $mapping
	 *   An optional map of extensions to their mimetypes, in the form:
	 *    - 'mimetypes': a list of mimetypes, keyed by an identifier,
	 *    - 'extensions': the mapping itself, an associative array in which
	 *      the key is the extension and the value is the mimetype identifier.
	 *
	 * @return
	 *   Returns a string containing the MIME type of the resource.
	 */
	public static function getMimeType($uri, $mapping = NULL)
	{
		return 'application/x-php';
	}

	/**
	 * Changes permissions of the resource.
	 *
	 * PHP lacks this functionality and it is not part of the official stream
	 * wrapper interface. This is a custom implementation for Drupal.
	 *
	 * @param $mode
	 *   Integer value for the permissions. Consult PHP chmod() documentation
	 *   for more information.
	 *
	 * @return
	 *   Returns TRUE on success or FALSE on failure.
	 */
	function chmod($mode)
	{
		//Not supported. We are read-only.
		return false;
	}

	/**
	 * Returns canonical, absolute path of the resource.
	 *
	 * Implementation placeholder. PHP's realpath() does not support stream
	 * wrappers. We provide this as a default so that individual wrappers may
	 * implement their own solutions.
	 *
	 * @return
	 *   Returns a string with absolute pathname on success (implemented
	 *   by core wrappers), or FALSE on failure or if the registered
	 *   wrapper does not provide an implementation.
	 */
	function realpath()
	{
		//Not supported. Database sources do not have a real path.
		assert(false);
		return false;
	}

	/**
	 * Support for fopen(), file_get_contents(), file_put_contents() etc.
	 *
	 * @param $uri
	 *   A string containing the URI to the file to open.
	 * @param $mode
	 *   The file mode ("r", "wb" etc.).
	 * @param $options
	 *   A bit mask of STREAM_USE_PATH and STREAM_REPORT_ERRORS.
	 * @param $opened_path
	 *   A string containing the path actually opened.
	 *
	 * @return
	 *   Returns TRUE if file was opened successfully.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-open.php
	 */
	public function stream_open($uri, $mode, $options, &$opened_path)
	{
		$this->uri = $uri;
		$url_info = parse_url($uri);
		if ($url_info === false)
		{
			return false;
		}
		
		if ($url_info['host'] === 'shared' /*/<file path>*/)
		{
			$this->handle = fopen(drupal_get_path('module', CVWOREPORT_MODULE) .
				'/' . self::SHARED_REPORTS_DIRECTORY . $url_info['path'], 'rb');
			if (!$this->handle)
			{
				return false;
			}
		}
		else if ($url_info['host'] === 'private' /*/<id>*/)
		{
			$row = db\select(CVWO_REPORT_SPECIAL_TABLE)
				->fields(CVWO_REPORT_SPECIAL_TABLE, array('report_text'))
				->condition('report_id', substr($url_info['path'], 1))
				->execute()
				->fetchAssoc();

			if ($row === false)
			{
				if (($options & STREAM_REPORT_ERRORS) !== 0)
				{
					trigger_error(t('Cannot find the private report @path', array(
						'@path' => $uri
					)), E_USER_WARNING);
				}
				return false;
			}

			$this->handle = $row['report_text'];
			$this->handle_pos = 0;
		}
		else
		{
			if (($options & STREAM_REPORT_ERRORS) !== 0)
			{
				trigger_error(t('Cannot open the path @path', array(
					'@path' => $uri
				)), E_USER_WARNING);
			}
			return false;
		}

		return true;
	}

	/**
	 * Support for flock().
	 *
	 * @param $operation
	 *   One of the following:
	 *   - LOCK_SH to acquire a shared lock (reader).
	 *   - LOCK_EX to acquire an exclusive lock (writer).
	 *   - LOCK_UN to release a lock (shared or exclusive).
	 *   - LOCK_NB if you don't want flock() to block while locking (not
	 *     supported on Windows).
	 *
	 * @return
	 *   Always returns true at the present time.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-lock.php
	 */
	public function stream_lock($operation)
	{
		if (is_resource($this->handle) &&
			in_array($operation, array(LOCK_SH, LOCK_EX, LOCK_UN, LOCK_NB)))
		{
			return flock($this->handle, $operation);
		}

		return true;
	}

	/**
	 * Support for fread(), file_get_contents() etc.
	 *
	 * @param $count
	 *   Maximum number of bytes to be read.
	 *
	 * @return
	 *   The string that was read, or FALSE in case of an error.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-read.php
	 */
	public function stream_read($count)
	{
		if (is_resource($this->handle))
		{
			return fread($this->handle, $count);
		}
		else
		{
			$result = substr($this->handle, $this->handle_pos, $count);
			$this->handle_pos += $count;
			return $result;
		}
	}

	/**
	 * Support for fwrite(), file_put_contents() etc.
	 *
	 * @param $data
	 *   The string to be written.
	 *
	 * @return
	 *   The number of bytes written (integer).
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-write.php
	 */
	public function stream_write($data)
	{
		//Cannot write.
		return 0;
	}

	/**
	 * Support for feof().
	 *
	 * @return
	 *   TRUE if end-of-file has been reached.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-eof.php
	 */
	public function stream_eof()
	{
		if (is_resource($this->handle))
		{
			return feof($this->handle);
		}
		else
		{
			return $this->handle_pos >= strlen($this->handle);
		}
	}

	/**
	 * Support for fseek().
	 *
	 * @param $offset
	 *   The byte offset to go to.
	 * @param $whence
	 *   SEEK_SET, SEEK_CUR, or SEEK_END.
	 *
	 * @return
	 *   TRUE on success.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-seek.php
	 */
	public function stream_seek($offset, $whence)
	{
		if (is_resource($this->handle))
		{
			// fseek returns 0 on success and -1 on a failure.
			// stream_seek   1 on success and  0 on a failure.
			return !fseek($this->handle, $offset, $whence);
		}
		else if ($whence === SEEK_CUR)
		{
			$this->handle_pos += $offset;
		}
		else if ($whence === SEEK_SET)
		{
			$this->handle_pos = $offset;
		}
		else //if ($whence === SEEK_END)
		{
			$this->handle_pos = strlen($this->handle) - $offset;
		}
	}

	/**
	 * Support for fflush().
	 *
	 * @return
	 *   TRUE if data was successfully stored (or there was no data to store).
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-flush.php
	 */
	public function stream_flush()
	{
		//No data to store.
		return true;
	}

	/**
	 * Support for ftell().
	 *
	 * @return
	 *   The current offset in bytes from the beginning of file.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-tell.php
	 */
	public function stream_tell()
	{
		if (is_resource($this->handle))
		{
			return ftell($this->handle);
		}
		else
		{
			return min($this->handle_pos, strlen($this->handle));
		}
	}

	/**
	 * Support for fstat().
	 *
	 * @return
	 *   An array with file status, or FALSE in case of an error - see fstat()
	 *   for a description of this array.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-stat.php
	 */
	public function stream_stat()
	{
		//Not always a file.
		return false;
	}

	/**
	 * Support for fclose().
	 *
	 * @return
	 *   TRUE if stream was successfully closed.
	 *
	 * @see http://php.net/manual/en/streamwrapper.stream-close.php
	 */
	public function stream_close()
	{
		if (is_resource($this->handle))
		{
			return fclose($this->handle);
		}
		
		return true;
	}

	/**
	 * Support for unlink().
	 *
	 * @param $uri
	 *   A string containing the URI to the resource to delete.
	 *
	 * @return
	 *   TRUE if resource was successfully deleted.
	 *
	 * @see http://php.net/manual/en/streamwrapper.unlink.php
	 */
	public function unlink($uri)
	{
		assert(false);
		return false;
	}

	/**
	 * Support for rename().
	 *
	 * @param $from_uri,
	 *   The URI to the file to rename.
	 * @param $to_uri
	 *   The new URI for file.
	 *
	 * @return
	 *   TRUE if file was successfully renamed.
	 *
	 * @see http://php.net/manual/en/streamwrapper.rename.php
	 */
	public function rename($from_uri, $to_uri)
	{
		assert(false);
		return false;
	}

	/**
	 * Gets the name of the directory from a given path.
	 *
	 * This method is usually accessed through drupal_dirname(), which wraps
	 * around the PHP dirname() function because it does not support stream
	 * wrappers.
	 * 
	 * This is a duplicate of Drupal's own dirname implementation in stream_wrappers.inc
	 *
	 * @param $uri
	 *   A URI or path.
	 *
	 * @return
	 *   A string containing the directory name.
	 *
	 * @see drupal_dirname()
	 */
	public function dirname($uri = NULL)
	{
		list($scheme, $target) = explode('://', $uri, 2);
		$target = $this->getTarget($uri);
		$dirname = dirname($target);

		if ($dirname == '.')
		{
			$dirname = '';
		}

		return $scheme . '://' . $dirname;
	}

	/**
	 * Support for mkdir().
	 *
	 * @param $uri
	 *   A string containing the URI to the directory to create.
	 * @param $mode
	 *   Permission flags - see mkdir().
	 * @param $options
	 *   A bit mask of STREAM_REPORT_ERRORS and STREAM_MKDIR_RECURSIVE.
	 *
	 * @return
	 *   TRUE if directory was successfully created.
	 *
	 * @see http://php.net/manual/en/streamwrapper.mkdir.php
	 */
	public function mkdir($uri, $mode, $options)
	{
		//Not supported.
		assert(false);
		return false;
	}

	/**
	 * Support for rmdir().
	 *
	 * @param $uri
	 *   A string containing the URI to the directory to delete.
	 * @param $options
	 *   A bit mask of STREAM_REPORT_ERRORS.
	 *
	 * @return
	 *   TRUE if directory was successfully removed.
	 *
	 * @see http://php.net/manual/en/streamwrapper.rmdir.php
	 */
	public function rmdir($uri, $options)
	{
		//Also not supported.
		assert(false);
		return false;
	}

	/**
	 * Support for stat().
	 *
	 * @param $uri
	 *   A string containing the URI to get information about.
	 * @param $flags
	 *   A bit mask of STREAM_URL_STAT_LINK and STREAM_URL_STAT_QUIET.
	 *
	 * @return
	 *   An array with file status, or FALSE in case of an error - see fstat()
	 *   for a description of this array.
	 *
	 * @see http://php.net/manual/en/streamwrapper.url-stat.php
	 */
	public function url_stat($uri, $flags)
	{
		//Not supported.
		return false;
	}

	/**
	 * Support for opendir().
	 *
	 * @param $uri
	 *   A string containing the URI to the directory to open.
	 * @param $options
	 *   Unknown (parameter is not documented in PHP Manual).
	 *
	 * @return
	 *   TRUE on success.
	 *
	 * @see http://php.net/manual/en/streamwrapper.dir-opendir.php
	 */
	public function dir_opendir($uri, $options)
	{
		$this->uri = $uri;
		$url_info = parse_url($uri);
		if ($url_info === false)
		{
			return false;
		}
		
		if ($url_info['host'] === 'shared' /*<file path>*/)
		{
			$this->handle = opendir(drupal_get_path('module', CVWOREPORT_MODULE) .
				'/' . self::SHARED_REPORTS_DIRECTORY);
			if (!$this->handle)
			{
				return false;
			}
		}
		else if ($url_info['host'] === 'private' /*<id>*/)
		{
			$this->handle = db\select(CVWO_REPORT_SPECIAL_TABLE)
				->fields(CVWO_REPORT_SPECIAL_TABLE, array('report_id'))
				->execute();

			if ($this->handle === false)
			{
				if (($options & STREAM_REPORT_ERRORS) !== 0)
				{
					trigger_error(t('Cannot list private reports'), E_USER_WARNING);
				}
				return false;
			}

			$this->handle_pos = 0;
		}
		else
		{
			if (($options & STREAM_REPORT_ERRORS) !== 0)
			{
				trigger_error(t('Cannot open the path @path', array(
					'@path' => $uri
				)), E_USER_WARNING);
			}
			return false;
		}
		
		return true;
	}

	/**
	 * Support for readdir().
	 *
	 * @return
	 *   The next filename, or FALSE if there are no more files in the directory.
	 *
	 * @see http://php.net/manual/en/streamwrapper.dir-readdir.php
	 */
	public function dir_readdir()
	{
		if (is_resource($this->handle))
		{
			return readdir($this->handle);
		}
		else if (is_object($this->handle))
		{
			$result = $this->handle->fetchAssoc();
			if ($result === false)
			{
				return false;
			}

			return $result['report_id'];
		}
		else
		{
			$result = current($this->handle);
			next($this->handle);
			return $result;
		}
	}

	/**
	 * Support for rewinddir().
	 *
	 * @return
	 *   TRUE on success.
	 *
	 * @see http://php.net/manual/en/streamwrapper.dir-rewinddir.php
	 */
	public function dir_rewinddir()
	{
		if (is_resource($this->handle))
		{
			rewinddir($this->handle);
		}
		else if (is_object($this->handle))
		{
			//Uhh. Not possible.
		}
		else
		{
			reset($this->handle);
		}

		// We do not really have a way to signal a failure as rewinddir() does not
		// have a return value and there is no way to read a directory handler
		// without advancing to the next file.
		return TRUE;
	}

	/**
	 * Support for closedir().
	 *
	 * @return
	 *   TRUE on success.
	 *
	 * @see http://php.net/manual/en/streamwrapper.dir-closedir.php
	 */
	public function dir_closedir()
	{
		if (is_resource($this->handle))
		{
			closedir($this->handle);
		}

		// We do not really have a way to signal a failure as closedir() does not
		// have a return value.
		return TRUE;
	}
}
