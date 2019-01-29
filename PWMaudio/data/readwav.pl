
use Audio::Wav;
use Data::Dumper;

my $wav = new Audio::Wav;
my $details;
my $read;

if( 1 ) {
# read
$read = $wav -> read( 'Alarm08.wav' );
print " ", $read -> length_seconds(), " seconds long\n";
print " ", $read -> length_samples(), " samples\n";
print " ", $read -> length(), " bytes (audio data)\n";
$details = $read -> details();
print Data::Dumper->Dump([ $details ]);
my ($min, $max) = (0,0);
for(my $i=0; $i<22050; $i++ ) {
    my @channels = $read -> read();
    last unless @channels;
    $min = $channels[0] > $min ? $min : $channels[0];
    $max = $channels[0] < $max ? $max : $channels[0];
#    print $channels[$channel_id], "\n";
}
print "min=$min, max=$max of +-", 2**16/2, "\n";
}

if(1) {
# write one channel
my %out_details = map { $_ => $details -> {$_} } 'bits_sample', 'sample_rate';
$out_details{channels} = 1;
print Data::Dumper->Dump( [\%out_details] );
my $write = $wav -> write( 'output.wav', \%out_details );
my $channel_id = 0;
for(my $i=0; $i<22050; $i++ ) {
    my @channels = $read -> read();
    last unless @channels;
    $write -> write( $channels[$channel_id] );
    #print $channels[$channel_id], "\n";
}
$write -> finish();
}

# read written
$read = $wav -> read( 'output.wav' );
print "- ", $read -> length_seconds(), " seconds long\n";
print "- ", $read -> length_samples(), " samples\n";
print "- ", $read -> length(), " bytes (audio data)\n";
print Data::Dumper->Dump([ $read -> details() ]);
open(OF,">output.h") or die;
print OF "const uint16 sweep_wav[] PROGMEM = {\n";
for(my $i=0; $i<10000000; $i++ ) {
    my @channels = $read -> read();
    last unless @channels;
    $min = $channels[0] > $min ? $min : $channels[0];
    $max = $channels[0] < $max ? $max : $channels[0];
    printf OF "0x%.4s,\n", sprintf("%04x", $channels[0]);
    print $channels[$channel_id], "\n";
}
printf OF "0x0000 };\n";
close(OF);
print "min=$min, max=$max of +-", 2**16/2, "\n";


