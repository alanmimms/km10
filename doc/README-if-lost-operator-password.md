Just a quick follow-up on this. I was able to follow Mark's
instructions, as relayed by Rich, and get patch the monitor on my
instance and reset my password.

I did have to modify slightly in that, breaking on GOTSWM, I found
that the page containing CHKPSW was not (yet?) loaded. However,
breaking on SWPINI and following essentially the same procedure
yielded the desired results. That is, the exact sequence of steps:

	GO
	/e
	$w
	SWPINI$b
	dbugsw/2
	147$g
	(at the breakpoint...)
	$b
	CHKPSW/$<JRST RSKP
	$>
	1,,SWPINI$g

and then login as normal (^C log operator foo once TOPS-20 is up).
