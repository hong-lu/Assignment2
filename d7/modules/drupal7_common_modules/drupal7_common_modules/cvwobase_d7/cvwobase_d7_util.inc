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
	 * Escapes the given string to be printable where JavaScript strings are
	 * allowed.
	 *
	 * @param string $string The string to escape.
	 */
	function js_escape($string)
	{
		return
			str_replace('"', '\\"',
			str_replace('\'', '\\\'',
			str_replace("\r", "\\r\\\r",
			str_replace("\n", "\\n\\\n",
			str_replace("\r\n", "\\r\\\n\\\\r\n",
			str_replace('\\', '\\\\', $string))))));
	}

	/**
	 * Fits the original dimensions in the new bounding box, maintaining the
	 * aspect ratio of the original dimensions.
	 *
	 * Each array here is a binary array, containing the width as the first
	 * element and height as the second.
	 *
	 * @param array $src_dimensions An array containing the width and height of
	 *                              the source image.
	 * @param array $dst_dimensions An array containing the width and height of
	 *                              the destination bounding box. Either width
	 *                              or height, but not both, can be empty to
	 *                              indicate a "don't care"
	 * @return array An array containing the width and height of the image to
	 *               be scaled down to.
	 */
	function rescale_image($src_dimensions, $dst_dimensions)
	{
		$img_width = $src_dimensions[0];
		$img_height = $src_dimensions[1];
		$max_width = empty($dst_dimensions[0]) ? $img_width : $dst_dimensions[0];
		$max_height = empty($dst_dimensions[1]) ? $img_height : $dst_dimensions[1];

		$scale = min(
			//Take the side which requires more scaling.
			$max_width / $img_width,
			$max_height / $img_height,

			//The largest possible value is 1, so there's no upscaling.
			1);
		$scale_width = round($scale * $img_width);
		$scale_height = round($scale * $img_height);

		return array($scale_width, $scale_height);
	}
	
	
	/**
	 * Calculates the difference between now and the provided date, in years.
	 * 
	 * This rounds down, so can be used to compute age.
	 * 
	 * @param \DateTime $date The Date to compare.
	 * @return The number of years elapsed, rounded down.
	 */
	function calculate_age($dob)
	{
		if (empty($dob))
		{
			return NaN;
		}

		$now = new \DateTime();
		return $now->diff($dob)->y;
	}
	
	/**
	 * Call from hook_theme_registry_alter
	 * 
	 * See http://www.metachunk.com/blog/adding-module-path-drupal-7-theme-registry
	 * 
	 * @param string $mod_path The path to add.
	 */
	function add_path_to_theme_registry(&$theme_registry, $mod_path)
	{
		$theme_registry_copy = $theme_registry;	   // munge on a copy
		_theme_process_registry($theme_registry_copy, 'phptemplate', 'theme_engine', 'pow', $mod_path);
		$theme_registry		+= array_diff_key($theme_registry_copy, $theme_registry);
		$hooks				 = array('node');
		
		$insert_after_first_element = function(&$a, $element) {
			if (is_array($a))
			{
				$first_element = array_shift($a);
				array_unshift($a, $first_element, $element);
			}
		};

		foreach ($hooks as $h)
		{
			$insert_after_first_element($theme_registry[$h]['theme paths'], $mod_path);
		}
	}
}
