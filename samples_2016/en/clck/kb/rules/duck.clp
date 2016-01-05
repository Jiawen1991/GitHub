(defrule duck-data-missing
  "Detect cases where there is no DUCK data."
  ; IF the mode is health and the 'duck' check is enabled
  (object (is-a CONFIG) (name [config]) (clck-mode health)
          (clck-checks $? duck $?))
  ; AND a node instance with the role 'compute' or 'enhanced' exists
  (object (is-a NODE) (node_id ?n)
          (role $?role&:(member$ compute ?role)|:(member$ enhanced ?role)))
  ; AND there are no instances of the DUCK class
  (not (object (is-a DUCK) (node_id ?n)))
  =>
  ; THEN create a sign
  (make-instance of BOOLEAN_SIGN (node_id ?n)
                 (confidence 90) (severity 90)
                 (state observed) (value TRUE)
                 (id "duck-data-missing")
                 (remedy "duck-data-missing-remedy")))

(defrule duck-data-is-too-old
  "Identify instances where the most recent DUCK data should be
   considered too old."
  ; IF the mode is health and the 'duck' check is enabled
  (object (is-a CONFIG) (name [config]) (clck-mode health)
          (clck-checks $? duck $?) (data-age-threshold ?age-threshold))
  ; AND a node instance with the role 'compute' or 'enhanced' exists
  (object (is-a NODE) (node_id ?n)
          (role $?role&:(member$ compute ?role)|:(member$ enhanced ?role)))
  ; AND an instance of the DUCK class exists for a node with the same
  ; node_id
  ?o <- (object (is-a DUCK) (node_id ?n) (timestamp ?t))
  ; AND that instance is older than the allowed age threshold
  (test (< ?t (- (now) ?age-threshold)))
  =>
  ; THEN create a sign
  (bind ?age (round (- (now) ?t)))
  (make-instance of BOOLEAN_SIGN (node_id ?n)
                 (confidence 90) (severity 20)
                 (source ?o) (state observed) (value TRUE)
                 (id "duck-data-is-too-old")
                 (args (create$ ?age))))

(defrule duck-honking
  "If the duck honks like a goose, something serious has happened."
  ; IF the mode is health and the 'duck' check is enabled
  (object (is-a CONFIG) (name [config]) (clck-mode health)
          (clck-checks $? duck $?))
  ; AND a node instance with the role 'compute' or 'enhanced' exists
  (object (is-a NODE) (node_id ?n)
          (role $?role&:(member$ compute ?role)|:(member$ enhanced ?role)))
  ; AND an instance of the DUCK class exists for a node with the same
  ; node_id and with the sound 'honk'
  ?o <- (object (is-a DUCK) (node_id ?n) (sound honk))
  =>
  ; THEN create a sign
  (make-instance of BOOLEAN_SIGN (node_id ?n)
                 (confidence 100) (severity 100)
                 (source ?o) (state observed) (value TRUE)
                 (id "duck-honking")))

(defrule duck-less-than-three-quacks
  "Create a sign whenever the number of 'quacks' is less than 3."
  ; IF the mode is health and the 'duck' check is enabled
  (object (is-a CONFIG) (name [config]) (clck-mode health)
          (clck-checks $? duck $?))
  ; AND a node instance with the role 'compute' or 'enhanced' exists
  (object (is-a NODE) (node_id ?n)
          (role $?role&:(member$ compute ?role)|:(member$ enhanced ?role)))
  ; AND an instance of the DUCK class exists for a node with the same
  ; node_id and with the sound 'quack'
  ?o <- (object (is-a DUCK) (count ?c) (node_id ?n) (sound quack))
  ; AND the number of quacks is less than 3
  (test (< ?c 3))
  =>
  ; THEN create a sign
  (make-instance of COUNTER_SIGN (node_id ?n)
                 (confidence 90) (severity 50)
                 (source ?o) (state observed) (value ?c)
                 (id "duck-less-than-three-quacks")
                 (args (create$ ?c))))

(defrule duck-quack-count-is-not-consistent
  "Create a sign whenever the number of 'quacks' is not consistent."
  ; IF the mode is health and the 'duck' check is enabled
  (object (is-a CONFIG) (name [config]) (clck-mode health)
          (clck-checks $? duck $?))
  ; AND a node instance with the role 'compute' or 'enhanced' exists
  (object (is-a NODE) (node_id ?n)
          (role $?role&:(member$ compute ?role)|:(member$ enhanced ?role)))
  ; AND an instance of the DUCK class exists for a node with the same
  ; node_id and with the sound 'quack'
  ?o <- (object (is-a DUCK) (node_id ?n) (count ?c) (multiset_control TRUE)
                (sound quack))
  ; AND the fraction of nodes with the same quack count is less than 0.9
  (test (< (send ?o fraction (send ?o multiset-key) ?c) 0.9))
  =>
  (bind ?key (send ?o multiset-key))
  (bind ?fraction (- 1 (send ?o fraction ?key ?c)))
  (make-instance of BOOLEAN_SIGN (node_id ?n)
                 (confidence (* 100 ?fraction)) (severity 80)
                 (state observed) (source ?o) (value TRUE)
                 (id "duck-quack-count-is-not-consistent")
                 (args (create$ (* 100 (send ?o fraction ?key ?c)) ?c))))
