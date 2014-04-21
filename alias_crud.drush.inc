<?php

/**
 * @file
 * Implements alias file operations.
 *
 * With this command you can:
 * - create a new alias file
 * - get a value
 * - set a value
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

  $items['alias-delete'] = array(
    'description' => "Deletes an alias key",
    'arguments' => array(
      'domain' => 'The alias file.',
      'selector' => 'Path into value tree',
    ),
    'examples' => array(
      'drush example alias-delete example.com dev/root' => 'Deletes the root key from /dev.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );

  return $items;
}

function drush_alias_crud_alias_create_file($domain, $environment = 'dev') {
  $file = _drush_alias_crud_get_filename($domain);
  if (file_exists($file)) {
    drush_set_error("File exists: '$file'.");
    return;
  }


  $tree = array(
    $environment => array(),
  );
  // Set some convenience defaults.
  drush_log("Adding defaults for '$domain.$environment'.");
  _drush_alias_crud_scan_or_set($tree, "/$environment/root", getcwd());
  _drush_alias_crud_scan_or_set($tree, "/$environment/fqdn", "http://$domain");

  drush_log("Creating file for domain '$domain'.");
  _drush_alias_crud_put_contents($domain, $tree);
}

function drush_alias_crud_alias_get($domain, $selector = '/') {
  $tree = _drush_alias_crud_get_contents($domain);
  $subtree = & $tree;

  list($subtree, $paths) = _drush_alias_crud_scan_or_set($subtree, $selector);
  if (!empty($paths)) {
    $path = array_shift($paths);
    $value = $subtree[$path];
    drush_log("$selector = '$value'", 'ok');
  }
  else {
    if (is_array($subtree)) {
      drush_log("Keys found: " . join(',', array_keys($subtree)), 'ok');
    }
    else if (isset($subtree)) {
      drush_log($subtree, 'ok');
    }
    else {
      drush_log("No keys found for path: $selector", 'ok');

    }
  }
}

function drush_alias_crud_alias_set($domain, $selector, $value) {
  $file = _drush_alias_crud_get_filename($domain);
  if (!file_exists($file)) {
    drush_set_error("File does not exists: '$file'. Maybe create it first?");
    return;
  }

  $tree = _drush_alias_crud_get_contents($domain);

  _drush_alias_crud_scan_or_set($tree, $selector, $value);

  _drush_alias_crud_put_contents($domain, $tree, FALSE);
}

function drush_alias_crud_alias_delete($domain, $selector) {
  $file = _drush_alias_crud_get_filename($domain);
  if (!file_exists($file)) {
    drush_set_error("File does not exists: '$file'. Maybe create it first?");
    return;
  }

  $tree = _drush_alias_crud_get_contents($domain);

  if (empty($tree)) {
    return;
  }
  var_dump($tree);
  _drush_alias_crud_scan_or_set($tree, $selector, NULL, TRUE);
  var_dump($tree);
  _drush_alias_crud_put_contents($domain, $tree, FALSE);
}

function _drush_alias_crud_scan_or_set(&$subtree, $selector, $value = NULL, $delete = FALSE) {
  if (substr($selector, 0, 1) !== '/') {
    $selector = '/' . $selector;
  }

  drush_log("Processing : '$selector'");

  if ($selector === '/') {
    $paths = array();
  }
  else {
    $paths = explode('/', $selector);
    array_shift($paths);
  }
  if ($delete && !count($paths)) {
    drush_set_error("You must give a path to delete an entry.");
    return FALSE;
  }
  while (!empty($paths)) {
    $path = array_shift($paths);
    // Create a value
    if ($value) {
      // Add value somewhere in the tree.
      if (empty($paths)) {
        // We are done so this may not be an array.
        if (isset($subtree[$path]) && is_array($subtree[$path])) {
          drush_set_error("Your selector has keys: " . join(",", array_keys($subtree[$path])));
          return FALSE;
        }
        drush_log("Expanding tree : '$path/'");
        drush_log("Adding value: '$value'");
        $subtree[$path] = $value;
      }
      else {
        // We need do dive deeper.
        if (!isset($subtree[$path])) {
          drush_log("Expanding tree : '$path/'");
          $subtree[$path] = array();
        }
        else if (!is_array($subtree[$path])) {
          drush_set_error("You selector is already set to a value before '/" . join("/", $paths) . "'. Maybe delete it first?");
          return FALSE;
        }
      }
    }
    // Delete entry before moving into it
    if (empty($paths) && $delete) {
      unset($subtree[$path]);
      return;
    }
    // Move into sub tree
    if (isset($subtree[$path])) {
      $subtree = & $subtree[$path];
    }

    if (!is_array($subtree) && !empty($paths)) {
      drush_set_error("Selector does not apply.");
      return;
    }
  }
  return array($subtree, $paths);
}

function _drush_alias_crud_get_contents($domain) {
  $file = _drush_alias_crud_get_filename($domain);
  drush_log("Reading file " . $file);
  // Read file
  $contents = file_get_contents($file);
  $lines = explode(PHP_EOL, $contents);
  // Drop <?php for eval
  array_shift($lines);

  // Make sure we get the aliases
  $lines[] = 'return $aliases;';
  $contents = implode(PHP_EOL, $lines);

  // use evil eval.
  return eval($contents);
}

function _drush_alias_crud_put_contents($domain, $tree, $create = FALSE) {
  $file = _drush_alias_crud_get_filename($domain);
  if ($create) {
    drush_log("Creating: " . $file);
  }
  else {
    drush_log("Updating: " . $file);

  }
  $contents = array();
  $contents[] = '<?php' . PHP_EOL;
  $contents[] = '# updated by ' . __FILE__;
  $contents[] = '$aliases = ' . var_export($tree, TRUE) . ';' . PHP_EOL;
  file_put_contents($file, implode(PHP_EOL, $contents));
}

/**
 * Generate file name based on given domain.
 *
 * The file is places in .drush directory
 *
 * @param $domain
 * @return string
 */
function _drush_alias_crud_get_filename($domain) {
  return drush_server_home() . '/.drush/' . $domain . ".aliases.drushrc.php";
}
