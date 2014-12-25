/***********************
version drupal 6

AUTHOR: Ying Bo
PERSON IN CHARGE:  Wang Sha
TESTING: This module is not used in the live system
Code Review: Yu Zhan/2013.05.20, 

MODULE: Data Validation Module

DESCRIPTION: 
 - This module is developed to check data integrity. 
 It's not used in the Lions system because all the missing data have been filled.
 Nevertheless, this module is very useful for the staff to detect and modify 
 missing/wrong data.

DESCRIPTION OF INCLUDE FILES:
 - lions_data_validation_api.inc contains functions to detect and update incorrect records.
 - lions_data_validation.inc contains the main form
 - lions_data_validation_constants.php contains constants used in this module

************************/
version drupal 7

AUTHOR: Yu Zhan
PERSON IN CHARGE:  Chunmun
TESTING: This module is not used in the live system, but as a debug & checking module used by cvwo team.
Code Review: Name/Date, 

MODULE: Data Validation Module

DESCRIPTION: 
 - Same
 - Fixed the bug of pager, by changing the self-implemented to the build in default one.

DESCRIPTION OF INCLUDE FILES:
 - same