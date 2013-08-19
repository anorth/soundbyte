//
//  SASViewController.m
//  AudioServer
//
//  Created by Alex on 31/07/13.
//  Copyright (c) 2013 Soundbyte. All rights reserved.
//

#import "SASViewController.h"

@interface SASViewController ()

@property (strong, nonatomic) IBOutlet UILabel *ipAddressLabel;
@property (strong, nonatomic) IBOutlet UILabel *statusLabel;

@end

@implementation SASViewController

- (void)viewDidLoad {
  [super viewDidLoad];
}

- (void)didReceiveMemoryWarning {
  [super didReceiveMemoryWarning];
}

- (void)setIpAddress:(NSString *)ipAddr port:(NSString *)port {
  self.ipAddressLabel.text = [NSString stringWithFormat:@"%@ : %@", ipAddr, port];
}

- (void)statusChanged:(NSString *)status {
  self.statusLabel.text = status;
}

@end
