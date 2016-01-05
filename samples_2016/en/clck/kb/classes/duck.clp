(defclass DUCK
  "This class corresponds to the 'duck' node rating tool."
  (is-a BASE_NODE BASE_TIMESTAMP DATABASE MULTISET)
  (role concrete)
  (pattern-match reactive)

  (slot count (type INTEGER) (default 1))
  (slot sound (type SYMBOL) (allowed-values honk quack) (default honk)))

(defmessage-handler DUCK init after ()
  "Add MULTISET key / value pairs.  Skip non-quacks."
  (if (eq ?self:sound quack) then
    (send ?self add (send ?self multiset-key) ?self:count)))

(defmessage-handler DUCK multiset-key ()
  "Generate a distinct key for each node architecture, role, and
   subcluster combination."
  ; defaults
  (bind ?architecture x86_64)
  (bind ?role compute)
  (bind ?subcluster default)

  (bind ?ins (find-instance ((?n NODE)) (eq ?n:node_id ?self:node_id)))
  (if (= (length ?ins) 1) then
    (bind ?i (nth$ 1 ?ins))
    (bind ?architecture (send ?i get-architecture))
    (bind ?subcluster (send ?i get-subcluster))
    (if (member$ compute (send ?i get-role)) then
      (bind ?role compute)
      else (if (member$ enhanced (send ?i get-role)) then
             (bind ?role enhanced))))

  (bind ?key (sym-cat (class ?self) + ?subcluster + ?role + ?architecture))
  (return ?key))
