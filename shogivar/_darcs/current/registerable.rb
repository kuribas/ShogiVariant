module Registerable
   def self.append_features(mod)
      mod.const_set(:REGISTERED_CLASSES, [])

      mod.module_eval do
         def self.register_class
            self.const_get(:REGISTERED_CLASSES) << self
         end

         def self.registered_classes
            self.const_get(:REGISTERED_CLASSES)
         end
      end
   end
end

# example:
#
# class SuperTest
#    include Registerable
# end
#
# class SubTest < SuperTest
#    register_class
# end
#
# puts SuperTest.registered_classes.inspect
# => [SubTest]
