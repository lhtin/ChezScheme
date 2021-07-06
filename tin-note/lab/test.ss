(interaction-environment
  (copy-environment
    (environment
      '(rnrs)
      '(rnrs eval) 
      '(rnrs mutable-pairs) 
      '(rnrs mutable-strings) 
      '(rnrs r5rs))
    #t))