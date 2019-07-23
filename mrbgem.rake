MRuby::Gem::Specification.new('mruby-libgba') do |spec|
	spec.license = 'Public domain'
	spec.author  = 'Lanza Schneider'
	spec.summary = 'Libgba to mruby binding'
	
	spec.cc.include_paths << "#{ENV['DEVKITPRO']}/libgba/include"
end
