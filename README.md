drush_alias_crud
================

Run crud operations on a drush alias file

This is a __temporary repository__ as I hope this ends into drush core.

Create the alias file
--
```
drush alias-create-file drupal
```

Set some defaults
--
```
drush alias-set drupal /d8/root /home/clemens/sites/drupal/d8
drush alias-set drupal /d8/uri http://drupal.d8
```

Set the database
--
```
drush alias-set drupal /d8/databases/default/default/database drupal_d8
drush alias-set drupal /d8/databases/default/default/username drupal_d8
drush alias-set drupal /d9/databases/default/default/password drupal_d8
drush alias-set drupal /d8/databases/default/default/host localhost
```
