<?php

/**
 * @file
 * Implements alias file operations.
 */

/**
 * Implements hook_drush_command().
 */
function alias_crud_drush_command() {
  $items = array();

  $items['alias-create-file'] = array(
    'description' => "Create drush alias file",
    'arguments' => array(
      'domain' => 'Domain name for alias file',
    ),
    'options' => array(
      'environment' => array(
        'description' => dt("environment type. Defaults to 'dev'"),
      ),
    ),
    'examples' => array(
      'drush alias-create-file example.com' => 'Create new alias file with initial dev.',
      'drush alias-create-file example.com staging' => 'Create new alias file with initial staging.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );

  $items['alias-get'] = array(
    'description' => "get keys or value for given alias file and path value",
    'arguments' => array(
      'domain' => 'The alias file.',
      'selector' => 'Path into alias file.',
    ),
    'examples' => array(
      'drush example.com alias-list' => 'List top level path entries.',
      'drush example.com alias-list dev' => 'List entries for dev',
      'drush example.com alias-list dev/databases' => 'List entries for dev/databases',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );

  $items['alias-set'] = array(
    'description' => "Sets an alias value",
    'arguments' => array(
      'domain' => 'The alias file.',
      'selector' => 'Path into value tree',
      'value' => 'New value to set',
    ),
    'examples' => array(
      'drush example alias-set example.com dev/root /home/user/www' => 'Sets the root for the dev alias.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );

  return $items;
}

function drush_alias_crud_alias_create_file($domain, $environment = 'dev') {
  $file = _drush_alias_crud_get_filename($domain, $create = FALSE);

  if (FALSE === $file) {
    return;
  }
  $tree = array(
    $environment => array(),
  );

  drush_log("Creating file for domain '$domain'.");
  _drush_alias_crud_put_contents($domain, $tree, TRUE);
}

function drush_alias_crud_alias_get($domain, $selector = '/') {
  $tree = _drush_alias_crud_get_contents($domain);
  $subtree = & $tree;

  list($subtree, $paths) = _drush_alias_crud_scan($subtree, $selector);
  if (!empty($paths)) {
    $path = array_shift($paths);
    $value = $subtree[$path];
    drush_log("$selector = '$value'", 'ok');
  }
  else {
    if (count($subtree)) {
      drush_log("Keys found: " . join(',', array_keys($subtree)), 'ok');
    }
    else {
      drush_log("No keys found for path: $selector", 'ok');

    }
  }
}

function drush_alias_crud_alias_set($domain, $selector, $value) {
  $tree = _drush_alias_crud_get_contents($domain);

  _drush_alias_crud_scan($tree, $selector, $value);

  _drush_alias_crud_put_contents($domain, $tree);
}

function _drush_alias_crud_scan(&$subtree, $selector, $value = NULL) {
  if (substr($selector, 0, 1) !== '/') {
    $selector = '/' . $selector;
  }
  drush_log("Listing : '$selector'");

  if ($selector === '/') {
    $paths = array();
  }
  else {
    $paths = explode('/', $selector);
    array_shift($paths);
  }
  while (!empty($paths)) {
    $path = array_shift($paths);
    if ($value) {
      if (empty($paths)) {
        $subtree[$path] = $value;
      }
      else {
        if (!isset($subtree[$path])) {
          $subtree[$path] = array();
        }
      }
    }
    if (is_array($subtree[$path])) {
      $subtree = & $subtree[$path];
    }
    else {
      // value found
      array_unshift($paths, $path);
      return array(&$subtree, $paths);
    }
  }
  return array($subtree, $paths);
}

function _drush_alias_crud_get_contents($domain, $exists = TRUE) {
  $file = _drush_alias_crud_get_filename($domain, !$exists);
  if ($file && $exists) {
    drush_log("Reading file " . $file);
    $contents = file_get_contents($file);
    $lines = explode(PHP_EOL, $contents);
    array_shift($lines);
    $lines[] = 'return $aliases;';
    $contents = implode(PHP_EOL, $lines);
    return eval($contents);
  }
  if (FALSE === $file) {
    return array();
  }
}

function _drush_alias_crud_put_contents($domain, $tree, $create = FALSE) {
  $file = _drush_alias_crud_get_filename($domain, $create);
  if ($file) {
    if ($create) {
      drush_log("Creating: " . $file);
    }
    else {
      drush_log("Updating: " . $file);

    }
    $contents = '<?php' . PHP_EOL;
    $contents .= '$aliases = ' . var_export($tree, TRUE) . ';' . PHP_EOL;
    file_put_contents($file, $contents);
  }
}

function _drush_alias_crud_get_filename($domain, $create = FALSE) {
  $file = $domain . ".aliases.drushrc.php";
  if (file_exists($file) && $create) {
    drush_set_error(dt("Alias file $file already exists."));
    return FALSE;
  }

  return $file;
}
