# tengphp5

Teng PHP5 extension.

Teng is a general purpose templating engine (whence Teng). 

http://opensource.seznam.cz/teng/


## Code Example


Source file - hello.php:

```
<?php

  // We will send this variable to template
  $hello_string = "Hello World!";
  
  // initialize Teng engine with default data root
  $teng = teng_init();

  // create new data tree
  $data = teng_create_data_root();
  
  teng_add_fragment( $data, "fragment", array( "variable" => $hello_string ) );
  
  // generate page
  echo( teng_page_string( $teng, "hello.html", $data,
      array( "content_type" => "text/html",
             "encoding" => "ISO-8859-1" ) ) );

  // release data tree (not necessary, but good practice)
  teng_release_data( $data );

  // release teng engine (not necessary, boot good practice)
  teng_release( $teng );
?>

```

Template file - hello.html:

```

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
	<title>Teng Template example</title>
</head>
<body>

<?teng frag fragment?>
    <h1>${variable}</h1>
<?teng endfrag?>

</body>
</html>

```

Output from Teng:

```

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
	<title>Teng Template example</title>
</head>
<body>
	<h1>Hello World!</h1>
</body>
</html>

```





