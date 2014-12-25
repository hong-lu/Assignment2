<?php
/*
 * The MIT License
 *
 * Copyright 2013 Computing for Volunteer Welfare Organizations (CVWO),
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

namespace CVWO\Base {
	/**
	 * Implements a theme hook for image upload form fields.
	 *
	 * This will display the currently uploaded image if one exists, or the
	 * #value otherwise. You can specify the width and height of the displayed
	 * image by setting the #width and #height attributes.
	 *
	 * You can also set a blank image to show if no image is specified in
	 * #default_value. Set #template_image.
	 *
	 * To use: set the #theme attribute of your managed_file form element to
	 * 'form_image_upload'. The hook is written in cvwobase_d7.module.
	 *
	 * @param $variables
	 *   An associative array containing:
	 *   - element: A render element representing the file.
	 */
	function theme_form_image_upload($variables)
	{
		$element = $variables['element'];

		//Determine the path to the image to show.
		if (!empty($element['#file']))
		{
			$uri	 = file_create_url($element['#file']->uri);
			
			//Scale the uploaded file to fit the dimensions of our area
			if (!empty($element['#width']) || !empty($element['#height']))
			{
				$path = drupal_realpath($element['#file']->uri);
				$dimensions = getimagesize($path);
				$dimensions = rescale_image($dimensions,
					array($element['#width'], $element['#height']));
				$element['#width'] = $dimensions[0];
				$element['#height'] = $dimensions[1];
			}
		}
		else if (!empty($element['#default_value']))
		{
			$uri	 = $element['#default_value'];
		}
		else if (!empty($element['#template_image']))
		{
			$uri	 = $element['#template_image'];
		}

		$output = '';
		if (!empty($uri))
		{
			$output .= sprintf(
				'<img src="%s" style="width: %s; height: %s" />',
				$uri, empty($element['#width']) ?
					'auto' : (intval($element['#width']) . 'px'),
				empty($element['#height']) ?
					'auto' : (intval($element['#height']) . 'px'));
		}

		$container = array(
			'element' => array(
				'#children' => $output . theme_file_managed_file($variables),
				'#attributes' => array(
					'class' => array(
						'image_upload_wrapper'
					)
				)
			)
		);

		return theme_container($container);
	}
}
